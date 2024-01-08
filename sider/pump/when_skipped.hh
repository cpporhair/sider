//
//
//

#ifndef SIDER_PUMP_WHEN_SKIPPED_HH
#define SIDER_PUMP_WHEN_SKIPPED_HH

#include <utility>
#include <exception>

#include "sider/util/macro.hh"

namespace sider::pump {
    namespace _when_skipped {
        template <typename func_t>
        struct
        op {
            constexpr static bool when_skipped_op1 = true;
            func_t func;

            op(func_t&& f)
                : func(__fwd__(f)){
            }

            op(op&& o)noexcept
                : func(__fwd__(o.func)){
            }

            op(const op& o)noexcept
                : func(o.func){
            }
        };

        template <typename prev_t,typename func_t>
        struct
        __ncp__(sender) {

            using prev_type = prev_t;

            prev_t prev;
            func_t func;

            sender(prev_t&& s,func_t&& f)
                :prev(__fwd__(s))
                ,func(__fwd__(f)){
            }

            sender(sender&& o)noexcept
                :prev(__fwd__(o.prev))
                ,func(__fwd__(o.func)){
            }

            inline
            auto
            make_op() {
                return op<func_t>(__mov__(func));
            }

            template<typename context_t>
            auto
            connect(){
                return prev.template connect<context_t>().push_back(make_op());
            }
        };

        struct
        fn {
            template <typename sender_t,typename func_t>
            constexpr
            decltype(auto)
            operator ()(sender_t&& sender,func_t&& func) const {
                return _when_skipped::sender<sender_t, func_t>{
                    __fwd__(sender),
                    __fwd__(func)
                };
            }

            template <typename func_t>
            constexpr
            decltype(auto)
            operator ()(func_t&& func) const
            requires std::is_rvalue_reference_v<decltype(func)> {
                return bind_back<fn,func_t>(fn{}, __fwd__(func));
            }
        };
    }

    namespace pusher {
        template<uint32_t pos, typename scope_t>
        requires (pos < std::tuple_size_v<typename scope_t::element_type::op_tuple_type>)
        && (get_current_op_type_t<pos, scope_t>::when_skipped_op1)
        struct
        op_pusher<pos, scope_t> : op_pusher_base<pos, scope_t>  {
            template<typename context_t>
            static inline
            void
            push_skip(context_t& context, scope_t& scope) {
                decltype(auto) new_sender = std::get<pos>(scope->get_op_tuple()).func(context);
                auto new_scope = make_runtime_scope<runtime_scope_type::other>(
                    scope,
                    new_sender.template connect<context_t>().push_back(pop_pusher_scope_op<pos + 1>()).take()
                );
                op_pusher<0, __typ__(new_scope)>::push_value(context, new_scope);
            }
        };
    }

    namespace typed {

        template <typename context_t, typename sender_t, typename func_t>
        requires has_value_type<compute_sender_type<context_t, sender_t>>
        struct
        compute_sender_type<context_t, sider::pump::_when_skipped::sender<sender_t, func_t>> {
            using value_type = compute_sender_type<context_t, sender_t>::value_type;
        };
    }

    inline constexpr _when_skipped::fn when_skipped{};
}

#endif //SIDER_PUMP_WHEN_SKIPPED_HH
