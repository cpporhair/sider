//
//
//

#ifndef SIDER_KV_TASK_ON_SCHEDULER_HH
#define SIDER_KV_TASK_ON_SCHEDULER_HH

#include <memory>
#include <functional>

#include "spdk/nvme.h"

#include "sider/pump/op_pusher.hh"
#include "sider/pump/compute_sender_type.hh"

namespace sider::task {
    namespace _on_scheduler {
        template <typename scheduler_t>
        struct
        req {
            std::move_only_function<void(scheduler_t*)> cb;
        };

        template <typename scheduler_t>
        struct
        op {
            constexpr static bool on_scheduler_op = true;
            scheduler_t* scheduler;

            op(scheduler_t* s)
                : scheduler(s) {
            }

            op(op&& rhs)
                : scheduler(rhs.scheduler) {
            }

            template<uint32_t pos, typename context_t, typename scope_t>
            auto
            start(context_t &context, scope_t &scope) {
                return scheduler->schedule(
                    new req<scheduler_t> {
                        [context = context, scope = scope](scheduler_t* s) mutable {
                            pump::pusher::op_pusher<pos + 1, scope_t>::push_value(
                                context,
                                scope,
                                s
                            );
                        }
                    }
                );
            }
        };

        template<typename scheduler_t>
        struct
        sender {
            scheduler_t* scheduler;

            sender(scheduler_t* s)
                : scheduler(s) {
            }

            sender(sender &&rhs)
                : scheduler(rhs.scheduler) {
            }

            inline
            auto
            make_op(){
                return op<scheduler_t>(scheduler);
            }

            template<typename context_t>
            auto
            connect() {
                return pump::builder::op_list_builder<0>().push_back(make_op());
            }
        };
    }

    template <typename impl_t>
    struct
    on_scheduler_able {
    protected:
        void* cmd[16];
        spdk_ring* on_scheduler_req_queue;

        inline
        auto
        schedule(_on_scheduler::req<impl_t>* req) {
            assert(0 < spdk_ring_enqueue(on_scheduler_req_queue, (void**)&req, 1, nullptr));
        }

        void
        handle_on_scheduler() {
            size_t size = spdk_ring_dequeue(on_scheduler_req_queue, cmd, 16);
            for (uint32_t i = 0; i < size; ++i) {
                auto* req = (_on_scheduler::req<impl_t>*)cmd[i];
                req->cb(reinterpret_cast<impl_t *>(this));
            }
        }

    public:
        inline
        auto
        on_scheduler() {
            return _on_scheduler::sender(reinterpret_cast<impl_t *>(this));
        }

        on_scheduler_able()
            : on_scheduler_req_queue(spdk_ring_create(spdk_ring_type::SPDK_RING_TYPE_MP_SC, 4096, 0)){
        }
    };
}

namespace sider::pump::pusher {
    template<uint32_t pos, typename scope_t>
    requires (pos < std::tuple_size_v<typename scope_t::element_type::op_tuple_type>)
    && (get_current_op_type_t<pos, scope_t>::on_scheduler_op)
    struct
    op_pusher<pos, scope_t> : op_pusher_base<pos, scope_t> {
        template<typename context_t>
        static inline
        void
        push_value(context_t& context, scope_t& scope) {
            std::get<pos>(scope->get_op_tuple()).template start<pos>(context, scope);
        }
    };
}

namespace sider::pump::typed {
    template <typename context_t, typename scheduler_t>
    struct
    compute_sender_type<context_t, sider::task::_on_scheduler::sender<scheduler_t>> {
        using value_type = scheduler_t*;
        constexpr static bool multi_values = false;
        constexpr static bool has_value_type = true;
    };
}

#endif //SIDER_KV_TASK_ON_SCHEDULER_HH
