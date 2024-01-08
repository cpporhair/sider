//
//
//

#ifndef SIDER_PUMP_ANY_CASE_HH
#define SIDER_PUMP_ANY_CASE_HH

namespace sider::pump {

    namespace _any_case {
        template <typename func_t>
        struct
        op {
            constexpr static bool any_case_op = true;
            func_t func;

            op(func_t&& f)
                : func(__fwd__(f)){
            }

            op(op&& rhs)noexcept
                : func(__fwd__(rhs.func)) {
            }

            op(const op& rhs)
                : func(rhs.func) {
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

        struct fn {
            template <typename sender_t,typename func_t>
            constexpr
            decltype(auto)
            operator ()(sender_t&& sender,func_t&& func) const {
                return _any_exception::sender<sender_t, func_t>{
                    __fwd__(sender),
                    __fwd__(func)
                };
            }

            template <typename func_t>
            constexpr
            decltype(auto)
            operator ()(func_t&& func) const requires meta::must_rvalue<decltype(func)>::v {
                return bind_back<fn,func_t>(fn{}, __fwd__(func));
            }
        };
    }

    namespace pusher {
        template<uint32_t pos, typename scope_t>
        requires (pos < std::tuple_size_v<typename scope_t::element_type::op_tuple_type>)
        && (get_current_op_type_t<pos, scope_t>::any_case_op)
        struct
        op_pusher<pos, scope_t> : op_pusher_base<pos, scope_t>{

            template<typename context_t>
            static inline
            void
            push_exception(context_t& context, scope_t& scope, std::exception_ptr e) {
                try {
                    decltype(auto) new_sender = std::get<pos>(scope->get_op_tuple()).func(e);
                    auto new_scope = make_runtime_scope<runtime_scope_type::other>(
                        scope,
                        new_sender.template connect<context_t>().push_back(pop_pusher_scope_op<pos + 1>()).take()
                    );
                    op_pusher<0, __typ__(new_scope)>::push_value(context, new_scope);
                }
                catch (std::exception exp) {
                    op_pusher<pos + 1, scope_t>::push_exception(context, scope, std::current_exception());
                }
            }
        };
    }
}

#endif //SIDER_PUMP_ANY_CASE_HH
