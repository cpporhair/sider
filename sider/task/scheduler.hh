//
//
//

#ifndef SIDER_DBMS_TASK_SCHEDULER_HH
#define SIDER_DBMS_TASK_SCHEDULER_HH

#include <functional>
#include <spdk/env.h>
#include <thread>
#include <set>

#include "sider/pump/compute_sender_type.hh"
#include "sider/pump/op_tuple_builder.hh"
#include "sider/pump/op_pusher.hh"
#include "coro/object.hh"
#include "coro/promise.hh"


namespace sider::task {

    namespace _timer {
        struct
        req {
            uint64_t timestamp;
            std::move_only_function<void()> cb;
        };

        template<typename scheduler_t>
        struct
        op {
            constexpr static bool task_op = true;
            scheduler_t *scheduler;
            uint64_t timestamp;

            template<uint32_t pos, typename context_t, typename scope_t>
            auto
            start(context_t& context, scope_t& scope) {
                scheduler->schedule(
                    new req {
                        timestamp,
                        [context = context, scope = scope]() mutable {
                            pump::pusher::op_pusher<pos + 1, scope_t>::push_value(context, scope);
                        }
                    }
                );
            }
        };

        template<typename scheduler_t>
        struct
        sender {
            uint64_t timestamp;
        public:
            explicit
            sender(scheduler_t *s, uint64_t ts) noexcept
                : scheduler(s)
                , timestamp(ts){
            }

            inline
            auto
            make_op() {
                return op<scheduler_t>(scheduler);
            }

            template<typename context_t>
            auto
            connect() {
                return pump::builder::op_list_builder<0>().push_back(make_op());
            }

            scheduler_t *scheduler;
        };
    }

    namespace _tasks {
        struct
        req {
            std::move_only_function<void()> cb;
        };

        template<typename scheduler_t>
        struct
        op {
            constexpr static bool task_op = true;
            scheduler_t *scheduler;

            template<uint32_t pos, typename context_t, typename scope_t>
            auto
            start(context_t& context, scope_t& scope) {
                scheduler->schedule(
                    new req {
                        [context = context, scope = scope]() mutable {
                            pump::pusher::op_pusher<pos + 1, scope_t>::push_value(context, scope);
                        }
                    }
                );
            }
        };

        template<typename scheduler_t>
        struct
        sender {
        public:
            explicit sender(scheduler_t *schd) noexcept: scheduler(schd) {}

            inline
            auto
            make_op() {
                return op<scheduler_t>(scheduler);
            }

            template<typename context_t>
            auto
            connect() {
                return pump::builder::op_list_builder<0>().push_back(make_op());
            }

            scheduler_t *scheduler;
        };
    }
}

namespace sider::pump::pusher {
    template<uint32_t pos, typename scope_t>
    requires (pos < std::tuple_size_v<typename scope_t::element_type::op_tuple_type>)
    && (get_current_op_type_t<pos, scope_t>::task_op)
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
    compute_sender_type<context_t, sider::task::_tasks::sender<scheduler_t>> {
        constexpr static bool has_value_type = false;
        constexpr static bool multi_values = false;
    };
}

namespace sider::task {
    struct
    scheduler {
        friend struct _tasks::op<scheduler>;
        friend struct _timer::op<scheduler>;
    public:
        uint32_t bind_core;
        spdk_ring* tasks_queue;
        spdk_ring* timer_queue;
        std::set<_timer::req*> timer_set;
        void* tmp[4096]{};
    private:
        inline
        auto
        schedule(_tasks::req* task) const{
            assert(0 < spdk_ring_enqueue(tasks_queue, (void**)&task, 1, nullptr));
        }

        inline
        auto
        schedule(_timer::req* task) const{
            assert(0 < spdk_ring_enqueue(timer_queue, (void**)&task, 1, nullptr));
        }

    public:
        explicit
        scheduler(uint32_t core)
            : tasks_queue(spdk_ring_create(spdk_ring_type::SPDK_RING_TYPE_MP_SC, 4096, -1))
            , timer_queue(spdk_ring_create(spdk_ring_type::SPDK_RING_TYPE_MP_SC, 1024, -1))
            , bind_core(core) {
        };

        auto
        get_scheduler() noexcept {
            return _tasks::sender<scheduler>{this};
        }

        void
        handle_tasks() {
            size_t size = spdk_ring_dequeue(tasks_queue, tmp, 4096);
            for (uint32_t i = 0; i < size; ++i) {
                auto* req = (_tasks::req*)tmp[i];
                req->cb();
                delete req;
            }
        }

        void
        handle_timer() {
            _timer::req *req = nullptr;

            while (0 == spdk_ring_dequeue(timer_queue, (void**)&req, 4096))
                timer_set.emplace(req);

            if (timer_set.empty())
                return;

            while (!timer_set.empty()) {
                if (now_ms() < (*timer_set.begin())->timestamp)
                    return;
                req = *timer_set.begin();
                timer_set.erase(timer_set.begin());
                req->cb();
                delete req;
            }
        }

        inline
        uint64_t
        now_ms() {
            return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        }

        inline
        auto
        delay(uint64_t ms) {
            return _timer::sender<scheduler>(this, now_ms() + ms);
        }

        void
        advance() {
            handle_timer();
            handle_tasks();
        }

        [[nodiscard]]
        bool
        busy() const {
            return false;
        }
    };
}

#endif //SIDER_DBMS_TASK_SCHEDULER_HH