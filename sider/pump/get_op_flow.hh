#ifndef SIDER_PUMP_GET_OP_FLOW_HH
#define SIDER_PUMP_GET_OP_FLOW_HH

#include "./bind_back.hh"
#include "./scope.hh"
#include "./op_pusher.hh"
#include "./compute_sender_type.hh"

namespace sider::pump {
    namespace _get_op_flow {
        struct
        op {
            constexpr static bool get_op_flow_op = true;
        };

        template <typename prev_t>
        struct
        sender {
            using prev_type = prev_t;
            prev_t prev;

            explicit
            sender(prev_t&& s)
                :prev(__fwd__(s)){
            }

            sender(sender&& o)noexcept
                :prev(__fwd__(o.prev)){
            }

            inline
            auto
            make_op() {
                return op{};
            }

            template<typename context_t>
            auto
            connect(){
                return prev.template connect<context_t>().push_back(make_op());
            }
        };

        struct
        fn {
            template <typename sender_t>
            constexpr
            decltype(auto)
            operator ()(sender_t&& sender) const {
                return _get_op_flow::sender<sender_t>{ __fwd__(sender) };
            }

            decltype(auto)
            operator ()() const {
                return bind_back<fn>(fn{});
            }
        };
    }

    namespace pusher {
        template<uint32_t pos, typename scope_t>
        requires (pos < std::tuple_size_v<typename scope_t::element_type::op_tuple_type>)
        && (get_current_op_type_t<pos, scope_t>::get_op_flow_op)
        struct
        op_pusher<pos, scope_t> : op_pusher_base<pos, scope_t> {
            template<typename context_t, typename ...value_t>
            static inline
            void
            push_value(context_t& context, scope_t& scope, value_t&& ...v) {
                op_pusher<pos + 1, scope_t>::push_value(context, scope, scope, __fwd__(v)...);
            }
        };
    }

    namespace typed {
        template<typename context_t, typename sender_t>
        requires compute_sender_type<context_t, sender_t>::has_Value_type
        struct
        compute_sender_type<context_t, sider::pump::_get_op_flow::sender<sender_t>> {
            constexpr static bool has_value_type = true;
            constexpr static bool multi_values = true;
            using value_type = std::tuple<typename compute_sender_type<context_t,sender_t>::value_type>;
        };

        template<typename context_t, typename sender_t>
        requires (!compute_sender_type<context_t, sender_t>::has_Value_type)
        struct
        compute_sender_type<context_t, sider::pump::_get_op_flow::sender<sender_t>> {
            constexpr static bool has_value_type = true;
            constexpr static bool multi_values = false;
            using value_type = std::tuple<typename compute_sender_type<context_t,sender_t>::value_type>;
        };
    }

    constexpr inline _get_op_flow::fn get_op_flow{};
}
#endif //SIDER_PUMP_GET_OP_FLOW_HH
