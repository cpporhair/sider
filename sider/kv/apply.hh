//

//

#ifndef SIDER_KV_APPLY_HH
#define SIDER_KV_APPLY_HH

#include "sider/pump/concurrent.hh"
#include "sider/pump/visit.hh"
#include "sider/pump/any_exception.hh"
#include "sider/pump/any_case.hh"
#include "sider/pump/generate.hh"
#include "sider/pump/get_context.hh"

#include "./batch/allocate_put_id.hh"
#include "./fs/allocate.hh"
#include "./fs/free.hh"
#include "./index/update.hh"
#include "./index/cache.hh"
#include "./nvme/put_page.hh"
#include "./data/exceptions.hh"
#include "./as_task.hh"


namespace sider::kv {

    auto
    merge_batch_and_allocate_page() {
        return pump::get_context<data::batch*>()
            >> pump::then([](data::batch* b, ...){
                return fs::allocate_data_page(b);
            })
            >> pump::flat();
    }

    auto
    update_index(data::batch* b){
        return pump::for_each(b->cache)
            >> pump::concurrent()
            >> pump::then([](data::key_value& kv){ return index::update(kv.file); })
            >> pump::flat()
            >> pump::reduce()
            >> pump::then([](bool b){ if (!b) throw data::update_index_failed(); });
    }

    auto
    free_page_when_error(data::write_span_list& list){
        return pump::catch_exception<data::write_data_failed>([&list](data::write_data_failed&& e) mutable {
            return pump::just()
                >> pump::generate(list.spans)
                >> pump::concurrent()
                >> pump::then([](data::write_span& s) { return fs::free_span(s); })
                >> pump::flat()
                >> pump::reduce();
        });
    }

    auto
    write_data(data::write_span_list& list){
        return pump::for_each(list.spans)
            >> pump::concurrent()
            >> pump::then(nvme::put_span)
            >> pump::flat()
            >> pump::all([](data::write_span& span){ return span.all_wrote(); })
            >> pump::then([](bool b){ if (!b) throw data::write_data_failed(); });
    }

    auto
    write_meta(fs::_allocate::leader_res* res){
        return pump::then([res]() { return fs::allocate_meta_page(res); })
            >> pump::flat()
            >> pump::then([](auto &&res) {
                if constexpr (std::is_same_v<fs::_metadata::leader_res *, __typ__(res)>) {
                    return pump::just() >> write_data(res->span_list) >> free_page_when_error(res->span_list);
                }
                else if constexpr (std::is_same_v<fs::_metadata::follower_res, __typ__(res)>) {
                    return pump::just(__fwd__(res));
                }
                else if constexpr (std::is_same_v<fs::_metadata::failed_res, __typ__(res)>) {
                    return pump::just(std::make_exception_ptr(new data::allocate_page_failed()));
                }
                else {
                    static_assert(std::is_same_v<fs::_metadata::failed_res, __typ__(res)>);
                    throw data::absolutely_not_code_block();
                }
            })
            >> pump::flat();
    }

    auto
    handle_exception(fs::_allocate::follower_res& res){
        return pump::catch_exception<data::write_data_failed>([](data::write_data_failed&& e){
            return pump::just();
        });
    }

    auto
    check_batch(data::batch* b) {
        return pump::then([b](...){ return !b->any_error(); });
    }

    auto
    cache_data_if_succeed(data::batch* b){
        return check_batch(b)
            >> pump::visit()
            >> pump::then([b](auto&& v){
                if constexpr (std::is_same_v<__typ__(v), std::true_type>)
                    return pump::just()
                        >> pump::generate(b->cache)
                        >> pump::concurrent()
                        >> pump::then([](data::key_value& kv){ return index::cache(kv.file); })
                        >> pump::flat()
                        >> pump::reduce();
                else
                    return pump::just();
            })
            >> pump::flat();
    }

    auto
    notify_follower(fs::_allocate::leader_res* res){
        return pump::then([res](...){
            for(decltype(auto) e : res->followers_req) {
                e->cb(fs::_allocate::follower_res{});
                delete e;
            }
            delete res;
        });
    }

    auto
    request_put_serial_number(data::batch* b) {
        return pump::then([b](){
                return batch::allocate_put_id(b);
            })
            >> pump::flat();
    }

    auto
    apply() {
        return pump::get_context<data::batch*>()
            >> pump::then([](data::batch* b){
                return pump::just()
                    >> request_put_serial_number(b)
                    >> update_index(b)
                    >> merge_batch_and_allocate_page()
                    >> pump::then([b](auto &&res) {
                        if constexpr (std::is_same_v<fs::_allocate::leader_res *, __typ__(res)>) {
                            return pump::just()
                                >> write_data(res->span_list)
                                >> write_meta(res)
                                >> free_page_when_error(res->span_list)
                                >> notify_follower(res);
                        }
                        else if constexpr (std::is_same_v<fs::_allocate::follower_res, __typ__(res)>) {
                            return pump::just(__fwd__(res));
                        }
                        else if constexpr (std::is_same_v<fs::_allocate::failed_res, __typ__(res)>){
                            return pump::just(std::make_exception_ptr(new data::allocate_page_failed()));
                        }
                        else {
                            static_assert(false);
                        }
                    })
                    >> pump::flat()
                    >> pump::ignore_all_exception()
                    >> cache_data_if_succeed(b);
            })
            >> pump::flat();
    }

}

#endif //SIDER_KV_APPLY_HH
