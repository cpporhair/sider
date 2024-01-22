//
//
//

#ifndef SIDER_PUMP_VISIT_HH
#define SIDER_PUMP_VISIT_HH

#include "./bind_back.hh"

namespace sider::pump {
    namespace _visit {
        namespace not_value {
            struct
            op {
                constexpr static bool visit_op_not_value = true;
            };

            template <typename prev_t>
            struct
            __ncp__(sender) {
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
        }

        namespace has_value {
            template <typename value_t>
            struct
            op {
                constexpr static bool visit_op_has_value = true;
                value_t value;
            };

            template <typename prev_t, typename value_t>
            struct
            __ncp__(sender) {
                using prev_type = prev_t;
                prev_t prev;
                value_t value;

                explicit
                sender(prev_t&& s, value_t&& v)
                    : prev(__fwd__(s))
                    , value(__fwd__(value)){
                }

                sender(sender&& o)noexcept
                    : prev(__fwd__(o.prev))
                    , value(__fwd__(o.value)) {
                }

                inline
                auto
                make_op() {
                    return op{__fwd__(value)};
                }

                template<typename context_t>
                auto
                connect(){
                    return prev.template connect<context_t>().push_back(make_op());
                }
            };
        }

        struct
        fn1 {
            template<typename prev_t, typename value_t>
            constexpr
            decltype(auto)
            operator()(prev_t &&prev, value_t &&value) const {
                return has_value::sender<prev_t, value_t>{
                    __fwd__(prev),
                    __fwd__(value)
                };
            }

            template <typename prev_t>
            constexpr
            decltype(auto)
            operator ()(prev_t&& prev) const {
                return not_value::sender<prev_t>{
                    __fwd__(prev)
                };
            }
        };

        struct
        fn {
            template <typename value_t>
            constexpr
            decltype(auto)
            operator ()(value_t&& value) const {
                return bind_back<fn1, value_t>(fn1{}, __fwd__(value));
            }
            decltype(auto)
            operator ()() const {
                return bind_back<fn1>(fn1{});
            }
        };
    }

    namespace pusher {
        template<uint32_t pos, typename scope_t>
        requires (pos < std::tuple_size_v<typename scope_t::element_type::op_tuple_type>)
        && (get_current_op_type_t<pos, scope_t>::visit_op_not_value)
        struct
        op_pusher<pos, scope_t> : op_pusher_base<pos, scope_t> {

            template<typename context_t>
            static inline
            auto
            do_push_value(context_t &context, scope_t &scope, bool b) {
                if (b)
                    op_pusher<pos + 1, scope_t>::push_value(context, scope, std::true_type{});
                else
                    op_pusher<pos + 1, scope_t>::push_value(context, scope, std::false_type{});
            }

            template <typename context_t, typename values_t>
            requires std::is_pointer_v<std::decay_t<values_t>>
            static inline
            auto
            do_push_value(context_t &context, scope_t &scope, values_t&& v) {
                if (v)
                    op_pusher<pos + 1, scope_t>::push_value(context, scope, v);
                else
                    op_pusher<pos + 1, scope_t>::push_value(context, scope, nullptr);
            }

            template <typename context_t, typename ...values_t>
            static inline
            auto
            do_push_value(context_t& context, scope_t& scope, std::variant<values_t...>&& v) {
                std::visit(
                    [&context, &scope](auto &&v) {
                        op_pusher<pos + 1, scope_t>::push_value(context, scope, __fwd__(v));
                    },
                    __fwd__(v)
                );
            }

            template <typename context_t, typename ...values_t>
            static inline
            auto
            do_push_value(context_t& context, scope_t& scope, values_t&& ...v) {
                static_assert(std::is_same_v<context_t, context_t>);
            }

            template <typename context_t, typename values_t>
            static inline
            auto
            push_value(context_t& context, scope_t& scope, values_t&& v) {
                return do_push_value(context, scope, __fwd__(v));
            }
        };

        template<uint32_t pos, typename scope_t>
        requires (pos < std::tuple_size_v<typename scope_t::element_type::op_tuple_type>)
        && (get_current_op_type_t<pos, scope_t>::visit_op_has_value)
        struct
        op_pusher<pos, scope_t> : op_pusher_base<pos, scope_t> {

            template<typename context_t>
            static inline
            auto
            do_push_value(context_t &context, scope_t &scope, bool b) {
                if (b)
                    op_pusher<pos + 1, scope_t>::push_value(context, scope, std::true_type{});
                else
                    op_pusher<pos + 1, scope_t>::push_value(context, scope, std::false_type{});
            }

            template<typename context_t, typename values_t>
            static inline
            auto
            do_push_value(context_t &context, scope_t &scope, bool b, values_t&& v) {
                if (b)
                    op_pusher<pos + 1, scope_t>::push_value(context, scope, std::true_type{}, __fwd__(v));
                else
                    op_pusher<pos + 1, scope_t>::push_value(context, scope, std::false_type{}, __fwd__(v));
            }

            template <typename context_t, typename visit_t, typename values_t>
            static inline
            auto
            do_push_value(context_t &context, scope_t &scope, visit_t* p, values_t&& v) {
                if (v)
                    op_pusher<pos + 1, scope_t>::push_value(context, scope, p, __fwd__(v));
                else
                    op_pusher<pos + 1, scope_t>::push_value(context, scope, (nullptr_t)nullptr, __fwd__(v));
            }

            template <typename context_t, typename ...visit_t, typename value_t>
            static inline
            auto
            do_push_value(context_t& context, scope_t& scope, const std::variant<visit_t...>& vi, value_t&& va) {
                std::visit(
                    [&context, &scope, va = __fwd__(va)](auto &&vi) {
                        op_pusher<pos + 1, scope_t>::push_value(context, scope, vi, __fwd__(va));
                    },
                    vi
                );
            }

            template <typename context_t, typename values_t>
            static inline
            auto
            push_value(context_t& context, scope_t& scope, values_t&& v) {
                auto &op = std::get<pos>(scope->get_op_tuple());
                return do_push_value(context, scope, op.value, __fwd__(v));
            }

            template <typename context_t>
            static inline
            auto
            push_value(context_t& context, scope_t& scope) {
                auto &op = std::get<pos>(scope->get_op_tuple());
                return do_push_value(context, scope, op.value);
            }
        };
    }

    namespace typed {
        template <typename context_t, typename sender_t>
        requires std::is_same_v<
            std::variant_alternative_t<0, typename compute_sender_type<context_t, sender_t>::value_type>,
            std::variant_alternative_t<0, typename compute_sender_type<context_t, sender_t>::value_type>
        >
        struct
        compute_sender_type<context_t, sider::pump::_visit::not_value::sender<sender_t>> {
            using value_type = std::variant_alternative_t<0, typename compute_sender_type<context_t, sender_t>::value_type>;
            constexpr static bool has_value_type = true;
        };

        template <typename context_t, typename sender_t>
        requires std::is_same_v<
            bool,
            typename compute_sender_type<context_t, sender_t>::value_type
        >
        struct
        compute_sender_type<context_t, sider::pump::_visit::not_value::sender<sender_t>> {
            using value_type = std::true_type;
            constexpr static bool has_value_type = true;
        };

        template<typename context_t, typename sender_t>
        requires std::is_pointer_v<typename compute_sender_type<context_t, sender_t>::value_type>
        struct
        compute_sender_type<context_t, sider::pump::_visit::not_value::sender<sender_t>> {
            using value_type = typename compute_sender_type<context_t, sender_t>::value_type;
            constexpr static bool has_value_type = true;
        };

        template <typename context_t, typename sender_t, typename value_t>
        requires std::is_same_v<
            std::variant_alternative_t<0, value_t>,
            std::variant_alternative_t<0, value_t>
        >
        struct
        compute_sender_type<context_t, sider::pump::_visit::has_value::sender<sender_t, value_t>> {
            using value_type = std::tuple<
                std::variant_alternative_t<0, typename compute_sender_type<context_t, sender_t>::value_type>,
                typename compute_sender_type<context_t, sender_t>::value_type
            >;
            constexpr static bool multi_values = true;
            constexpr static bool has_value_type = true;
        };

        template <typename context_t, typename sender_t, typename value_t>
        requires std::is_same_v<
            bool,
            value_t
        >
        struct
        compute_sender_type<context_t, sider::pump::_visit::has_value::sender<sender_t, value_t>> {
            using value_type = std::tuple<
                std::true_type,
                typename compute_sender_type<context_t, sender_t>::value_type
            >;
            constexpr static bool multi_values = true;
            constexpr static bool has_value_type = true;
        };
    }

    inline constexpr _visit::fn visit{};
}

#endif //SIDER_PUMP_VISIT_HH
