#ifndef SIDER_CUDA_COMMON_WAIT_STREAM_DONE_HH
#define SIDER_CUDA_COMMON_WAIT_STREAM_DONE_HH

#include "driver_types.h"
#include "sider/pump/bind_back.hh"
#include "sider/pump/op_pusher.hh"
#include "sider/pump/compute_sender_type.hh"
#include "sider/util/macro.hh"

namespace sider::cuda::common {
    namespace _wait_stream_done {
        struct
        op {
            constexpr static bool wait_stream_done = true;
            cudaStream_t stream;

            op(cudaStream_t s)
                : stream(s){
            }

            op(op&& o) noexcept
                : stream(__fwd__(o.stream)){
            }

            op(const op& o) noexcept
                : stream(o.stream){
            }
        };

        template <typename prev_t>
        struct
        sender {
            using prev_type = prev_t;
            prev_t prev;
            cudaStream_t stream;

            sender(prev_t&& p,cudaStream_t s)
                : prev(__fwd__(p))
                , stream(s){
            }

            sender(sender&& o)noexcept
                : prev(__fwd__(o.prev))
                , stream(__fwd__(o.stream)){
            }

            inline
            auto
            make_op() {
                return op(stream);
            }

            template<typename context_t>
            auto
            connect() {
                return prev.template connect<context_t>().push_back(make_op());
            }
        };

        struct
        fn {
            template<typename sender_t>
            constexpr
            decltype(auto)
            operator()(sender_t &&sender, cudaStream_t stream) const {
                return _wait_stream_done::sender<sender_t>{
                    __fwd__(sender),
                    stream
                };
            }

            template<typename cuda_stream_t>
            constexpr
            decltype(auto)
            operator ()(cuda_stream_t stream) const {
                return sider::pump::bind_back<fn, cuda_stream_t>(fn{}, stream);
            }
        };
    }

    constexpr inline _wait_stream_done::fn wait_stream_done{};
}

namespace pump::pusher {
    template<uint32_t pos, typename scope_t>
    requires (pos < std::tuple_size_v<typename scope_t::element_type::op_tuple_type>)
    && (get_current_op_type_t<pos, scope_t>::wait_stream_done)
    struct
    op_pusher<pos, scope_t> : op_pusher_base<pos, scope_t> {
        template <typename context_t>
        struct user_data {
            scope_t scope;
            context_t& context;
        };
        template<typename context_t, typename ...value_t>
        static inline
        void
        push_value(context_t& context, scope_t& scope, value_t&& ...v) {
            auto &op = std::get<pos>(scope->get_op_tuple());
            cudaStreamAddCallback(
                op.stream,
                [](cudaStream_t stream, cudaError_t status, void *data) {
                    auto *ud = (user_data<context_t> *) data;
                    op_pusher<pos + 1, scope_t>::push_value(ud->context, ud->scope);
                    delete ud;
                },
                (void *) (new user_data<context_t>{scope, context}),
                0
            );
        }
    };
}

namespace pump::typed {
    template<typename context_t, typename sender_t>
    requires compute_sender_type<context_t, sender_t>::has_value_type
    struct
    compute_sender_type<context_t, sider::cuda::common::_wait_stream_done::sender<sender_t>> {
        constexpr static bool has_value_type = false;
        constexpr static bool multi_values = false;
    };
}

#endif //SIDER_CUDA_COMMON_WAIT_STREAM_DONE_HH
