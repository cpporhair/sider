//

//

#ifndef SIDER_KV_START_BATCH_HH
#define SIDER_KV_START_BATCH_HH

#include "sider/pump/flat.hh"
#include "sider/pump/push_context.hh"
#include "sider/pump/pop_context.hh"
#include "sider/pump/get_context.hh"
#include "sider/pump/visit.hh"

#include "./batch/start.hh"
#include "./finish_batch.hh"
namespace sider::kv {

    template <uint32_t compile_id>
    auto
    start_batch_with_compile_id() {
        return pump::ignore_all_exception()
            >> pump::push_context_with_id<compile_id>(new data::batch())
            >> pump::get_context<data::batch*>()
            >> pump::then([](data::batch* batch, auto&& ...args){
                return batch::start(batch)
                    >> pump::forward_value(__fwd__(args)...);
            })
            >> pump::flat();
    }

    template <uint32_t compile_id>
    inline
    auto
    as_batch_with_compile_id(auto&& b) requires std::is_same_v<std::monostate, typename __typ__(b)::bind_back_flag>{
        return pump::then([b = __fwd__(b)](auto&& ...args) mutable {
                return pump::just()
                    >> start_batch_with_compile_id<compile_id>()
                    >> pump::forward_value(__fwd__(args)...)
                    >> __fwd__(b)
                    >> pump::ignore_all_exception()
                    >> finish_batch();
            })
            >> pump::flat();
    }

    template<uint32_t compile_id>
    inline
    auto
    as_batch_with_compile_id(auto &&b) {
        return pump::then([b = __fwd__(b)](auto &&...args) mutable {
                return pump::just()
                    >> start_batch_with_compile_id<compile_id>()
                    >> pump::forward_value(__fwd__(args)...)
                    >> pump::then(__fwd__(b))
                    >> pump::flat()
                    >> pump::ignore_all_exception()
                    >> finish_batch();
            })
            >> pump::flat();
    }


#define start_batch start_batch_with_compile_id<__COUNTER__>
#define as_batch as_batch_with_compile_id<__COUNTER__>
}

#endif //SIDER_KV_START_BATCH_HH
