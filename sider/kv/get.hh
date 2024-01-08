//

//

#ifndef SIDER_KV_GET_HH
#define SIDER_KV_GET_HH

#include "sider/pump/concurrent.hh"
#include "sider/pump/on.hh"
#include "sider/pump/generate.hh"

#include "./data/exceptions.hh"
#include "./index/get.hh"
#include "./nvme/read_page.hh"

namespace sider::kv {
    namespace _get {
        auto
        read_pages(data::data_file* f) {
            return pump::for_each(f->pages)
                >> pump::concurrent()
                >> pump::then([](data::data_page* p){ return nvme::get_page(p); })
                >> pump::flat()
                >> pump::reduce();
        }

        auto
        on_index_scheduler(const data::slice* key) {
            return pump::then([key](auto&& ...args){ return index::on_scheduler(key) >> pump::forward_value(args...); })
                >> pump::flat();
        }

        auto
        notify_waiters(index::_getter::pager_reader_res* res) {
            return pump::then([res](bool b){
                for(auto *v : res->ver->page_waiters) {
                    auto *req = (index::_getter::req *) v;
                    if (b)
                        req->cb(new index::_getter::pager_waiter_res(data::key_value(res->ver->file)));
                    else
                        req->cb(data::read_page_failed());
                    delete req;
                }
            });
        }

        auto
        get(data::batch* b, const char* k) {
            return index::get(data::make_slice(k), b->get_snapshot->get_serial_number(), b->last_free_sn)
                >> pump::then([b](auto&& res){;
                    if constexpr (std::is_same_v<__typ__(res), index::_getter::pager_reader_res*>)
                        return pump::just()
                            >> as_task()
                            >> read_pages(res->kv.file)
                            >> on_index_scheduler(res->key)
                            >> notify_waiters(res)
                            >> pump::then([b, res](){
                                runtime::g_task_info.nv_count++;
                                auto kv = __mov__(res->kv);
                                delete res;
                                return kv;
                            });
                    else if constexpr (std::is_same_v<__typ__(res), index::_getter::pager_waiter_res*>)
                        return pump::just()
                            >> pump::then([res](){
                                runtime::g_task_info.nv_count++;
                                auto kv = __mov__(res->kv);
                                delete res;
                                return kv;
                            });
                    else if constexpr (std::is_same_v<__typ__(res), index::_getter::not_fround_res>)
                        return pump::just()
                            >> pump::forward_value(data::key_value{nullptr});
                    else if constexpr (std::is_same_v<__typ__(res), data::read_page_failed>)
                        return pump::just(std::make_exception_ptr(__fwd__(res)));
                    else
                        throw data::absolutely_not_code_block();
                })
                >> pump::flat()
                >> pump::any_exception([](...){
                    return pump::just()
                        >> pump::forward_value(data::key_value{nullptr});
                });
        }
    }

    auto
    get(const char* k) {
        return pump::get_context<data::batch*>()
            >> pump::then([k](data::batch* b){
                return _get::get(b, k);
            })
            >> pump::flat();
    }

    inline
    auto
    get() {
        return pump::get_context<data::batch*>()
            >> pump::then([](data::batch* b, auto&& k){
                if constexpr (std::is_same_v<std::string, __typ__(k)>) {
                    return pump::just()
                        >> pump::with_context(__fwd__(k))(
                            pump::get_context<std::string, data::batch*>()
                                >> pump::then([](std::string& s, data::batch* b){
                                    return _get::get(b, s.c_str());
                                })
                                >> pump::flat()
                        );
                }
                if constexpr (std::is_same_v<const char *, __typ__(k)>) {
                    return pump::just()
                        >> pump::with_context(std::string(k))(_get::get(b, k));
                }
                else if constexpr (std::is_same_v<data::key_value, __typ__(k)>) {
                    return _get::read_pages(k.file);
                }
                else {
                    static_assert(std::is_same_v<std::string, __typ__(k)>);
                }
            })
            >> pump::flat();
    }
}

#endif //SIDER_KV_GET_HH
