//
//
//

#ifndef SIDER_PUMP_PUSH_CONTEXT_HH
#define SIDER_PUMP_PUSH_CONTEXT_HH


#include "./bind_back.hh"
#include "./op_pusher.hh"
#include "./op_tuple_builder.hh"
#include "./compute_sender_type.hh"

namespace sider::pump {


    namespace _push_context {

        template <uint32_t pos, typename ...content_t>
        struct
        op {
            constexpr static bool push_context_op = true;
            constexpr static uint32_t push_pos = pos;

            std::tuple<content_t...> contents;

            op(std::tuple<content_t...>&& c)
                : contents(__fwd__(c)){
            }

            op(op &&rhs)
                : contents(__fwd__(rhs.contents)){
            }

            op(const op &rhs)
                : contents(rhs.contents) {
            }

            template <uint64_t id, typename context_t>
            auto
            get_new_context(context_t& context) {
                return std::make_shared<pushed_context<id, context_t, content_t...>>(context, __mov__(contents));
            }
        };

        template <uint32_t pos, typename prev_t, typename ...content_t>
        struct
        __ncp__(sender) {
            using prev_type = prev_t;
            const static bool push_context_sender = true;
            const static uint32_t push_pos = pos;
            prev_t prev;
            std::tuple<content_t...> contents;

            sender(prev_t&& s, content_t&& ...c)
                : prev(__fwd__(s))
                , contents(__fwd__(c)...) {
            }

            sender(sender&& rhs) noexcept
                : prev(__fwd__(rhs.prev))
                , contents(__fwd__(rhs.contents)){
            }

            inline
            auto
            make_op() {
                return op<pos, content_t...>(__fwd__(contents));
            }

            template<typename context_t>
            auto
            connect() {
                return prev.template connect<context_t>().push_back(make_op());
            }
        };

        template <uint32_t pos>
        struct
        fn2 {
            template<typename sender_t, typename ...content_t>
            constexpr
            decltype(auto)
            operator()(sender_t &&sender, content_t&& ...content) const {
                return _push_context::sender
                    <
                        pos,//builder::compute_push_context_sender_pos_v<sender_t>,
                        sender_t,
                        content_t...
                    >
                    {
                        __fwd__(sender),
                        __fwd__(content)...
                    };
            }
        };

        template <uint32_t pos>
        struct
        fn {
            template<typename ...content_t>
            decltype(auto)
            operator ()(content_t&& ...content) const {
                __all_must_rval__(content);
                return bind_back<fn2<pos>, content_t...>(fn2<pos>{}, __fwd__(content)...);
            }
        };
    }

    namespace pusher {
        template<uint32_t pos, typename scope_t>
        requires (pos < std::tuple_size_v<typename scope_t::element_type::op_tuple_type>)
        && (get_current_op_type_t<pos, scope_t>::push_context_op_pushed)
        struct
        op_pusher<pos, scope_t> : op_pusher_base<pos, scope_t> {
            template<typename context_t, typename ...value_t>
            static inline
            void
            push_value(context_t& context, scope_t& scope, value_t&& ...v) {
                op_pusher<pos + 1, scope_t>::push_value(context, scope, __fwd__(v)...);
            }
        };

        template<uint32_t pos, typename scope_t>
        requires (pos < std::tuple_size_v<typename scope_t::element_type::op_tuple_type>)
        && (get_current_op_type_t<pos, scope_t>::push_context_op)
        struct
        op_pusher<pos, scope_t> : op_pusher_base<pos, scope_t>  {
            constexpr static uint64_t pushed_id = get_current_op_type_t<pos, scope_t>::push_pos;
            template<typename context_t, typename ...value_t>
            static inline
            void
            push_value(context_t& context, scope_t& scope, value_t&& ...v) {
                auto& op = std::get<pos>(scope->get_op_tuple());
                if constexpr (context_t::element_type::template has_id<pushed_id>()) {
                    sider::pump::pusher::op_pusher<pos + 1, __typ__(scope)>::push_value(context, scope, __fwd__(v)...);
                }
                else {
                    auto new_context = op.template get_new_context<pushed_id>(context);
                    //auto *new_scope = change_opt<pos, scope_t, _push_context::op_pushed>(scope, _push_context::op_pushed{});
                    sider::pump::pusher::op_pusher<pos + 1, __typ__(scope)>::push_value(new_context, scope, __fwd__(v)...);
                }
            }


            template<typename context_t>
            static inline
            void
            push_exception(context_t& context, scope_t& scope, std::exception_ptr e) {
                if constexpr (context_t::element_type::template has_id<pushed_id>()) {
                    sider::pump::pusher::op_pusher<pos + 1, __typ__(scope)>::push_exception(context, scope, e);
                }
                else {
                    auto new_context = std::get<pos>(scope->get_op_tuple()).template get_new_context<pushed_id>(context);
                    op_pusher<pos + 1, scope_t>::push_exception(new_context, scope, e);
                }
            }

            template<typename context_t>
            static inline
            void
            push_skip(context_t& context, scope_t& scope) {
                if constexpr (context_t::element_type::template has_id<pushed_id>()) {
                    sider::pump::pusher::op_pusher<pos + 1, __typ__(scope)>::push_skip(context, scope);
                }
                else {
                    auto new_context = std::get<pos>(scope->get_op_tuple()).template get_new_context<pushed_id>(context);
                    op_pusher<pos + 1, scope_t>::push_skip(new_context, scope);
                }
            }

            template<typename context_t>
            static inline
            void
            push_done(context_t& context, scope_t& scope) {
                if constexpr (context_t::element_type::template has_id<pushed_id>()) {
                    sider::pump::pusher::op_pusher<pos + 1, __typ__(scope)>::push_done(context, scope);
                }
                else {
                    auto new_context = std::get<pos>(scope->get_op_tuple()).template get_new_context<pushed_id>(context);
                    op_pusher<pos + 1, scope_t>::push_done(new_context, scope);
                }
            }
        };
    }

    namespace typed {

        template <typename context_t, uint32_t pos, typename sender_t, typename ...content_t>
        requires has_value_type<compute_sender_type<context_t, sender_t>>
        struct
        compute_sender_type<context_t, sider::pump::_push_context::sender<pos, sender_t, content_t...>> {
            using value_type    = compute_sender_type<context_t, sender_t>::value_type;
        };

        template <typename context_t, uint32_t pos, typename sender_t, typename ...content_t>
        struct
        compute_sender_type<context_t, sider::pump::_push_context::sender<pos, sender_t, content_t...>> {
        };

        template <typename context_t, uint32_t pos, typename sender_t, typename ...content_t>
        struct
        compute_context_type<context_t, sider::pump::_push_context::sender<pos, sender_t, content_t...>> {
            using xxxx = compute_context_type<context_t, sender_t>::type;
            using type = std::shared_ptr<pushed_context<pos, xxxx, content_t...>>;
        };
    }

    template<uint32_t pos>
    inline constexpr _push_context::fn<pos> push_context_with_id{};
    #define push_context push_context_with_id<__COUNTER__>


}

#endif //SIDER_PUMP_PUSH_CONTEXT_HH
