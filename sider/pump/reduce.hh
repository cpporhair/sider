#ifndef SIDER_PUMP_REDUCE_HH
#define SIDER_PUMP_REDUCE_HH

namespace sider::pump {

    inline constexpr bool reduce_done = true;
    inline constexpr bool reduce_continue = false;

    namespace _reduce {
        template <typename result_t, typename func_t>
        struct
        op {
            using result_type = result_t;
            constexpr static bool is_reduce_op = true;
            result_t result;
            func_t func;

            op(result_t&& r, func_t&& f)
                : result(__fwd__(r))
                , func(__fwd__(f)) {
            }

            op(op&& rhs)
                : result(__fwd__(rhs.result))
                , func(__fwd__(rhs.func)){
            }

            op(const op& rhs)
                : result(rhs.result)
                , func(rhs.func){
            }

            template<typename ...value_t>
            auto
            do_run(value_t&& ...v) {
                if constexpr (std::is_void_v<__typ__(func(result, __fwd__(v)...))>) {
                    func(result, __fwd__(v)...);
                    return reduce_continue;
                }
                else if constexpr (std::is_same_v<bool, __typ__(func(result, __fwd__(v)...))>) {
                    return func(result, __fwd__(v)...);
                }
                else {
                    static_assert(
                        std::is_same_v<bool, __typ__(func(result, __fwd__(v)...))>,
                        "reduce function must return bool or void"
                    );
                }
            }

            auto
            take(){
                return __mov__(result);
            }

            template<typename ...value_t>
            auto
            run(value_t&& ...v) {
                do_run(__fwd__(v)...);
            }
        };


        template <typename prev_t, typename result_t, typename func_t>
        struct
        reduce_sender {
            using prev_type = prev_t;

            prev_t prev;
            func_t func;
            result_t result;

            reduce_sender(prev_t&& p, result_t&& r, func_t&& f)
                : func(__fwd__(f))
                , result(r)
                , prev(__fwd__(p)){
            }

            reduce_sender(reduce_sender&& rhs)
                : func(__fwd__(rhs.func))
                , result(__fwd__(rhs.result))
                , prev(__fwd__(rhs.prev)){
            }

            inline
            auto
            make_op() {
                return op<result_t, func_t>(__mov__(result), __mov__(func));
            }

            template<typename context_t>
            auto
            connect(){
                auto builder = prev.template connect<context_t>();
                return builder.push_reduce_op(reduce_carrier<__typ__(builder)::get_reduce_op_next_pos(), __typ__(make_op())>(make_op()));
            }
        };

        struct
        reduce_fn {
            template<typename sender_t, typename result_t, typename func_t>
            constexpr
            decltype(auto)
            operator()(sender_t &&prev, result_t&& res, func_t &&func) const {
                return reduce_sender<sender_t, result_t, func_t>{
                    __fwd__(prev),
                    __fwd__(res),
                    __fwd__(func)
                };
            }

            template <typename result_t, typename func_t>
            constexpr
            decltype(auto)
            operator ()(result_t&& res, func_t&& func) const {
                __must_rval__(res);
                __must_rval__(func);
                return bind_back<reduce_fn, result_t, func_t>(reduce_fn{}, __fwd__(res), __fwd__(func));
            }

            decltype(auto)
            operator ()() const {
                return this->operator()(
                    bool(true),
                    [](bool& b, auto&& ...v){
                        if constexpr (sizeof...(v) > 0) {
                            if constexpr (std::is_same_v<__typ__(v)..., std::monostate> ||
                                          std::is_same_v<__typ__(v)..., std::exception_ptr>) {
                                b = false;
                            }
                        }
                    }
                );
            }
        };

        template <typename prev_t, template<typename...> class container_t>
        struct
        to_container_sender {
            using prev_type = prev_t;

            prev_t prev;

            to_container_sender(prev_t&& p)
                : prev(__fwd__(p)){
            }

            to_container_sender(to_container_sender&& rhs)
                : prev(__fwd__(rhs.prev)){
            }

            template<typename context_t>
            inline
            auto
            make_op() {
                __out_type_name__(prev_t);
                static_assert(typed::has_value_type<typed::compute_sender_type<context_t, prev_t>>,"aaaaaaaaa");
            }

            template<typename context_t>
            requires typed::has_value_type<typed::compute_sender_type<context_t, prev_t>>
            inline
            auto
            make_op() {
                using result_t = std::variant<
                    std::monostate,
                    std::exception_ptr,
                    typename typed::compute_sender_type<context_t, prev_t>::value_type
                >;

                auto func = [](container_t<result_t>& res, auto&& ...v) mutable {
                    static_assert(sizeof...(v) > 0, "to container no value_type");
                    res.emplace_back(result_t(__fwd__(v)...));

                };
                return op<container_t<result_t>, __typ__(func)>(container_t<result_t>(), __mov__(func));
            }

            template<typename context_t>
            auto
            connect(){
                auto builder = prev.template connect<context_t>();
                return builder.push_reduce_op(
                    reduce_carrier<
                        __typ__(builder)::get_reduce_op_next_pos(),
                        __typ__(make_op<context_t>())
                    >(
                        make_op<context_t>()
                    )
                );
            }
        };

        template <template<typename...> class container_t >
        struct
        to_container_fn {
            template<typename sender_t>
            constexpr
            decltype(auto)
            operator()(sender_t &&prev) const {

                return to_container_sender<sender_t, container_t>{
                    __fwd__(prev)
                };
            }

            decltype(auto)
            operator ()() const {
                return bind_back<to_container_fn>(to_container_fn{});
            }
        };
    }

    namespace pusher {
        template <uint32_t pos,typename scope_t>
        requires (pos < std::tuple_size_v<typename scope_t::element_type::op_tuple_type>)
        && (get_current_op_type_t<pos, scope_t>::is_reduce_op)
        struct
        op_pusher<pos, scope_t> : op_pusher_base<pos, scope_t> {

            template <typename context_t, typename ...values_t>
            static
            void
            do_set_value(context_t& context, scope_t& scope, values_t&& ...v) {
                auto &op = std::get<pos>(scope->get_op_tuple());
                op.run(__fwd__(v)...);
                op_pusher<pos - 1, scope_t>::poll_next(context, scope);
            }

            template <typename context_t, typename ...values_t>
            static inline
            void
            push_value(context_t& context, scope_t& scope, values_t&& ...v) {
                do_set_value(context, scope, __fwd__(v)...);
            }

            template <typename context_t>
            static inline
            auto
            push_exception(context_t& context, scope_t& scope, std::exception_ptr e) {
                return do_set_value(context, scope, __mov__(e));
            }

            template <typename context_t>
            static inline
            auto
            push_done(context_t& context, scope_t& scope) {
                auto &op = std::get<pos>(scope->get_op_tuple());
                auto loop_scope = pop_to_loop_starter(scope);
                op_pusher<__typ__(op)::pos, __typ__(loop_scope)>::push_value(context, loop_scope, op.take());
            }

            template <typename context_t>
            static inline
            auto
            push_skip(context_t& context, scope_t& scope) {
                return do_set_value(context, scope, std::monostate());
            }
        };
    }

    namespace typed {
        template <typename context_t, typename sender_t, typename result_t, typename func_t>
        struct
        compute_sender_type<context_t, sider::pump::_reduce::reduce_sender<sender_t, result_t, func_t>> {
            using value_type = result_t;
            constexpr static bool multi_values = false;
            constexpr static bool has_value_type = true;
        };

        template <typename context_t, typename prev_t, template<typename...> class container_t>
        struct
        compute_sender_type<context_t, sider::pump::_reduce::to_container_sender<prev_t, container_t>> {
            using value_type = container_t<std::variant<
                std::monostate,
                std::exception_ptr,
                typename compute_sender_type<context_t,prev_t>::value_type
            >>;
            constexpr static bool multi_values = false;
        };
    }

    inline constexpr _reduce::reduce_fn reduce{};

    template<typename func_t>
    auto
    all(func_t&& f) {
        return reduce(
            bool(true),
            [f = __fwd__(f)](bool& b, auto&& ...v){
                if constexpr (sizeof...(v) == 1) {
                    using x = std::tuple_element_t<0, std::tuple<__typ__(v)...>>;
                    if constexpr (std::is_same_v<x, std::monostate> || std::is_same_v<x, std::exception_ptr>)
                        b = false;
                    else
                        b = f(__fwd__(v)...);
                }
                else {
                    b = f(__fwd__(v)...);
                }
                /*
                if constexpr (sizeof...(v) > 0) {
                    if constexpr (std::is_same_v<__typ__(v)..., std::monostate> ||
                                  std::is_same_v<__typ__(v)..., std::exception_ptr>) {
                        b = false;
                    }
                }
                else {
                    b = f(__fwd__(v)...);
                }
                */
            }
        );
    }

    inline
    auto
    all() {
        return all([](bool b) { return b; });
    }

    static
    auto
    count() {
        return reduce(
            uint64_t(0),
            [](uint64_t & c, auto&& ...v){
                c++;
            }
        );
    }

    template <template<typename...> class container_t >
    inline constexpr _reduce::to_container_fn<container_t> to_container{};

    inline constexpr _reduce::to_container_fn<std::vector> to_vector{};
}

#endif //SIDER_PUMP_REDUCE_HH
