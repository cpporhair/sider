//
//
//

#ifndef SIDER_PUMP_TASK_SCHEDULER_HH
#define SIDER_PUMP_TASK_SCHEDULER_HH
#include <thread>
#include <future>
#include <list>

namespace sider::pump {
    namespace _task_scheduler {

        template <typename scheduler_t>
        struct
        op {
            constexpr static bool task_scheduler_op = true;
            scheduler_t* sche;
            uint32_t i;

            template<uint32_t pos, typename context_t, typename scope_t>
            auto
            set_value(context_t& context, scope_t& scope) {
                sche->list.emplace_back(
                    [context = context, scope, v = i]() mutable {
                        pusher::op_pusher<pos, scope_t>::push_value(context, scope);
                    }
                );
            }
        };

        template <typename scheduler_t>
        struct
        sender {
            scheduler_t* scheduler;;

            inline
            auto
            make_op() {
                return op<scheduler_t>(scheduler);
            }

            template<typename context_t>
            auto
            connect() {
                return builder::op_list_builder<0>().push_back(make_op());
            }
        };

        struct
        fn {
            template <typename prev_t, typename scheduler_t>
            constexpr
            decltype(auto)
            operator ()(prev_t&& prev, scheduler_t* sche) const {
                return sender<scheduler_t>{sche};
            }

            template <typename scheduler_t>
            constexpr
            decltype(auto)
            operator ()(scheduler_t* sche) const {
                return bind_back<fn, scheduler_t, uint32_t>{fn{}, sche};
            }
        };

        struct
        scheduler {
            std::list<std::move_only_function<void()>> list;
            auto
            get_scheduler(){
                return sender<scheduler>{this};
            }

            auto
            forward(){
                if (!list.empty()) {
                    list.front()();
                    list.pop_front();
                }
            }
        };
    }

    namespace pusher {
        template<uint32_t pos, typename scope_t>
        requires (pos < std::tuple_size_v<typename scope_t::element_type::op_tuple_type>)
        && (get_current_op_type_t<pos, scope_t>::task_scheduler_op)
        struct
        op_pusher<pos, scope_t> : op_pusher_base<pos, scope_t>  {

            template<typename context_t>
            static inline
            void
            push_value(context_t& context, scope_t& scope) {
                std::get<pos>(scope->get_op_tuple()).template set_value<pos + 1>(context, scope);
            }
        };
    }

    namespace typed {
        template <typename context_t, typename scheduler_t>
        struct
        compute_sender_type<context_t, sider::pump::_task_scheduler::sender<scheduler_t>> {
            constexpr static bool has_value_type = false;
        };
    }


    using task_scheduler = _task_scheduler::scheduler;
}
#endif //SIDER_TEST_SCHEDULE_HH
