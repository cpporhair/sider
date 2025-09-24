//
//
//

#ifndef PUMP_TEST_SCHEDULE_HH
#define PUMP_TEST_SCHEDULE_HH
#include <thread>
#include <future>
#include <list>

namespace pump {
    namespace _test_schedule {

        template <typename scheduler_t>
        struct
        op {
            constexpr static bool test_schedule_op = true;
            scheduler_t* sche;
            uint32_t i;

            template<uint32_t pos, typename context_t, typename scope_t>
            auto
            set_value(context_t& context, scope_t& scope) {
                sche->list.emplace_back(
                    [context = context, scope, v = i](uint32_t arg) mutable {
                        pusher::op_pusher<pos, scope_t>::push_value(context, scope, v + arg);
                    }
                );
            }
        };

        template <typename scheduler_t>
        struct
        sender {
            scheduler_t* sche;
            uint32_t i;

            inline
            auto
            make_op() {
                return op<scheduler_t>(sche,i);
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
            operator ()(prev_t&& prev, scheduler_t* sche, uint32_t v) const {
                return sender<scheduler_t>{sche, v};
            }

            template <typename scheduler_t>
            constexpr
            decltype(auto)
            operator ()(scheduler_t* sche, uint32_t v) const {
                return bind_back<fn, scheduler_t, uint32_t>{fn{}, sche, v};
            }
        };

        struct
        scheduler {
            std::list<util::ncpy_func<void(int)>> list;
            auto
            get_scheduler(uint32_t i){
                return sender<scheduler>{this, i};
            }

            auto
            forward(){
                if (!list.empty()) {
                    list.front()(1);
                    list.pop_front();
                }
            }
        };
    }

    namespace pusher {
        template<uint32_t pos, typename scope_t>
        requires (pos < std::tuple_size_v<typename scope_t::element_type::op_tuple_type>)
        && (get_current_op_type_t<pos, scope_t>::test_schedule_op)
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
        template <typename context_t, typename sender_t>
        struct
        compute_sender_type<context_t, pump::_test_schedule::sender<sender_t>> {
            using value_type = uint32_t;
        };
    }


    using test_scheduler = _test_schedule::scheduler;
}
#endif //MONISM_TEST_SCHEDULE_HH
