//
//
//

#ifndef SIDER_PUMP_FLAT_HH
#define SIDER_PUMP_FLAT_HH


#include <utility>
#include <exception>

#include "./submit.hh"
#include "./op_pusher.hh"
#include "./then.hh"

namespace sider::pump {
    namespace _flat{
        struct
        op {
            constexpr static bool flat_op = true;
        };

        template <typename prev_t>
        struct
        __ncp__(sender) {
            using prev_type = prev_t;

            prev_t prev;

            explicit
            sender(prev_t s)
                :prev(__fwd__(s)) {
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
                return _flat::sender<sender_t>{
                    __fwd__(sender)
                };
            }

            /*
            template <typename func_t>
            requires std::is_function_v<func_t>
            constexpr
            decltype(auto)
            operator ()(func_t&& sender) const {
                return _flat::sender<func_t>{
                    __fwd__(sender)
                };
            }
            */

            decltype(auto)
            operator ()() const {
                return bind_back<fn>(fn{});
            }
        };
    }

    namespace pusher {
        template<uint32_t pos, typename scope_t>
        requires (pos < std::tuple_size_v<typename scope_t::element_type::op_tuple_type>)
        && (get_current_op_type_t<pos, scope_t>::flat_op)
        struct
        op_pusher<pos, scope_t> : op_pusher_base<pos, scope_t>{
            template <typename context_t, typename sender_t>
            static inline
            void
            push_value(context_t& context, scope_t& scope, sender_t&& sender) {

                auto new_scope = make_runtime_scope<runtime_scope_type::other>(
                    scope,
                    sender.template connect<context_t>().push_back(pop_pusher_scope_op<pos + 1>()).take()
                );

                op_pusher<0, __typ__(new_scope)>::push_value(context, new_scope);
            }

            template <typename context_t, typename ...sender_t>
            static inline
            void
            push_value(context_t& context, scope_t& scope, std::variant<sender_t...>&& any) {
                std::visit(
                    [&context, &scope](auto&& sender) mutable {
                        op_pusher::push_value(context, scope, __fwd__(sender));
                    },
                    any
                );
            }
        };
    }

    namespace typed {
        template <typename context_t, typename sender_t>
        requires
        has_value_type<
            compute_sender_type<
                context_t,
                typename compute_sender_type<
                    context_t,
                    sender_t
                >::value_type
            >
        >
        struct
        compute_sender_type<context_t, sider::pump::_flat::sender<sender_t>> {
            using sender_type   = compute_sender_type<context_t, sender_t>::value_type;
            using value_type    = compute_sender_type<context_t, sender_type>::value_type;
            constexpr static bool multi_values = false;
            constexpr static bool has_value_type = true;
        };

        template <typename context_t, typename sender_t>
        requires
        std::is_same_v<
            std::variant_alternative_t<0,typename compute_sender_type<context_t,sender_t>::value_type>,
            std::variant_alternative_t<0,typename compute_sender_type<context_t,sender_t>::value_type>
        >
        &&
        has_value_type<
            compute_sender_type<
                context_t,
                std::variant_alternative_t<
                    0,
                    typename compute_sender_type<
                        context_t,
                        sender_t
                    >::value_type
                >
            >
        >
        struct
        compute_sender_type<context_t, sider::pump::_flat::sender<sender_t>> {
            //__out_type_name__(sender_t);
            using sender_type   = std::variant_alternative_t<
                0,
                typename compute_sender_type<
                    context_t,
                    sender_t
                >::value_type
            >;
            using value_type    = compute_sender_type<context_t, sender_type>::value_type;
            constexpr static bool multi_values = false;
            constexpr static bool has_value_type = true;
        };

        template <typename context_t, typename sender_t>
        requires
        (!has_value_type<
            compute_sender_type<
                context_t,
                typename compute_sender_type<
                    context_t,
                    sender_t
                >::value_type
            >
        >)
        struct
        compute_sender_type<context_t, sider::pump::_flat::sender<sender_t>> {
            constexpr static bool has_value_type = false;
            constexpr static bool multi_values = false;
        };
    }

    inline constexpr _flat::fn flat{};

    template <typename func_t>
    inline
    auto
    flat_map(func_t&& f) {
        return transform(__fwd__(f)) >> flat();
    }
}
#endif //SIDER_PUMP_FLAT_HH
