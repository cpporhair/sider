//

//

#ifndef SIDER_KV_FINISH_DB_HH
#define SIDER_KV_FINISH_DB_HH

#include "sider/pump/any_exception.hh"

#include "./batch/publish.hh"

namespace sider::kv {
    auto
    publish() {
        return pump::get_context<data::batch*>()
            >> pump::then([](data::batch* b) {
                assert(b->put_snapshot != nullptr);
                return batch::publish(b);
            })
            >> pump::flat();
    }

    auto
    release() {
        return pump::get_context<data::batch*>()
            >> pump::then([](data::batch* b) {
                assert(b->put_snapshot == nullptr);
                b->get_snapshot->release();
            });
    }

    auto
    finish_batch() {
        return pump::get_context<data::batch*>()
            >> pump::then([](data::batch* b){
                return b->put_snapshot != nullptr;
            })
            >> pump::visit()
            >> pump::then([](auto&& res){
                if constexpr (std::is_same_v<__typ__(res), std::false_type>)
                    return pump::just() >> release();
                else
                    return pump::just() >> publish();
            })
            >> pump::flat()
            >> pump::get_full_context_object()
            >> pump::then([](auto& ctx){
                static_assert(
                    __typ__(ctx)::element_type::template has_type<data::batch*>, "Maybe the current batch object is not at the top of the context");
            })
            >> pump::get_context<data::batch*>()
            >> pump::then([](data::batch* b){
                if(b->put_snapshot != nullptr){
                    b->put_snapshot = nullptr;
                }
                delete b;
            })
            >> pump::pop_context();
    }
}

#endif //SIDER_KV_FINISH_DB_HH
