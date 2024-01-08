//
//
//

#ifndef SIDER_PUMP_GET_CONTEXT_HH
#define SIDER_PUMP_GET_CONTEXT_HH
#include "./bind_back.hh"

namespace sider::pump {
    namespace _get_context {
        template <typename ...env_t>
        struct
        op {
            constexpr static bool get_context_op = true;

            constexpr static bool is_get_all = (sizeof...(env_t) == 0);

            template <typename context_t>
            inline
            decltype(auto)
            get_from_context(context_t& context) {
                return get_all_from_context<context_t, env_t...>(context);
            }
        };

        template <typename prev_t,typename ...env_t>
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
                return op<env_t...>{};
            }

            template<typename context_t>
            auto
            connect(){
                return prev.template connect<context_t>().push_back(make_op());
            }
        };

        template <typename ...env_t>
        struct
        fn {
            template <typename sender_t>
            constexpr
            decltype(auto)
            operator ()(sender_t&& sender) const {
                return _get_context::sender<sender_t,env_t...>{ __fwd__(sender) };
            }

            decltype(auto)
            operator ()() const {
                return bind_back<fn<env_t...>>(fn<env_t...>{});
            }
        };
    }

    namespace pusher {
        template<uint32_t pos, typename scope_t>
        requires (pos < std::tuple_size_v<typename scope_t::element_type::op_tuple_type>)
        && (get_current_op_type_t<pos, scope_t>::get_context_op)
        struct
        op_pusher<pos, scope_t> : op_pusher_base<pos, scope_t> {
            template<typename context_t, typename ...value_t>
            requires get_current_op_type_t<pos, scope_t>::is_get_all
            static inline
            void
            push_value(context_t& context, scope_t& scope, value_t&& ...v) {
                op_pusher<pos + 1, scope_t>::push_value(context, scope, context, __fwd__(v)...);
            }

            template<typename context_t, typename ...value_t>
            static inline
            void
            push_value(context_t& context, scope_t& scope, value_t&& ...v) {
                auto &op = std::get<pos>(scope->get_op_tuple());
                return std::apply(
                [...v = __fwd__(v), &context, &scope](auto&& ...args) mutable {
                    op_pusher<pos + 1, scope_t>::push_value(context, scope, __fwd__(args)..., __fwd__(v)...);
                },
                op.get_from_context(context)
                );
            }
        };
    }

    namespace typed {
        template<typename context_t, typename sender_t>
        concept sender_has_multi_value
        = has_value_type<compute_sender_type<context_t, sender_t>>
          && mul_value_type<compute_sender_type<context_t, sender_t>>;

        template<typename context_t, typename sender_t>
        concept sender_has_single_value
        = has_value_type<compute_sender_type<context_t, sender_t>>
          && (!mul_value_type<compute_sender_type<context_t, sender_t>>);

        template<typename context_t, typename sender_t, typename ...env_t>
        requires sender_has_single_value<context_t, sender_t>
        struct
        compute_sender_type<context_t, sider::pump::_get_context::sender<sender_t,env_t...>> {
            constexpr static bool multi_values= true;
            using value_type = std::tuple<
                env_t&...,
                typename compute_sender_type<
                    context_t,
                    sender_t
                >::value_type
            >;
        };

        template<typename context_t, typename sender_t, typename ...env_t>
        requires sender_has_multi_value<context_t, sender_t>
        struct
        compute_sender_type<context_t, sider::pump::_get_context::sender<sender_t,env_t...>> {
            constexpr static bool multi_values= true;
            using value_type = tuple_push_front<
                env_t &...,
                typename compute_sender_type<
                    context_t,
                    sender_t
                >::value_type
            >::type;
        };

        template<typename context_t, typename sender_t, typename ...env_t>
        struct
        compute_sender_type<context_t, sider::pump::_get_context::sender<sender_t,env_t...>> {
            using value_type = std::tuple<env_t&...>;
            constexpr static bool multi_values = true;
            constexpr static bool has_value_type = true;
        };

        template<typename context_t, typename sender_t, typename env_t>
        struct
        compute_sender_type<context_t, sider::pump::_get_context::sender<sender_t,env_t>> {
            using value_type = env_t&;
            constexpr static bool multi_values = false;
            constexpr static bool has_value_type = true;
        };

        template<typename context_t, typename sender_t>
        struct
        compute_sender_type<context_t, sider::pump::_get_context::sender<sender_t>> {
            //using value_type = context_t&;
            using value_type = compute_context_type<context_t, sender_t>::type&;
            constexpr static bool multi_values = false;
            constexpr static bool has_value_type = true;
        };

    }

    template <typename ...env_t>
    inline constexpr _get_context::fn<env_t...> get_context{};

    inline constexpr _get_context::fn<> get_full_context_object{};
}
#endif //SIDER_PUMP_GET_CONTEXT_HH
