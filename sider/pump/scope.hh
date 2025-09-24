//
//
//

#ifndef PUMP_SCOPE_HH
#define PUMP_SCOPE_HH

#include <cstdint>
#include <memory>
#include <atomic>

namespace pump {

    enum struct
    runtime_scope_type {
        root,
        stream_starter,
        other
    };

    template <typename op_tuple_t>
    struct
    root_scope {
        constexpr static runtime_scope_type scope_type = runtime_scope_type::root;
        using op_tuple_type = op_tuple_t;
        op_tuple_t op_tuple;
        auto&
        get_op_tuple() {
            return op_tuple;
        }

        constexpr static
        uint32_t
        get_scope_level_id() {
            return 0;
        }

        auto
        get_scope_type() {
            return scope_type;
        }

        root_scope(op_tuple_t&& opt)
            : op_tuple(__fwd__(opt)) {
        }
    };

    template <runtime_scope_type type, typename op_tuple_t, typename base_t>
    struct
    runtime_scope {
        constexpr static runtime_scope_type scope_type = type;

        using op_tuple_type = op_tuple_t;

        op_tuple_t op_tuple;
        base_t base_scope;

        constexpr static
        uint32_t
        get_scope_level_id() {
            return base_t::get_scope_level_id() + 1;
        }

        auto&
        get_op_tuple() {
            return op_tuple;
        }

        auto
        get_scope_type() {
            return scope_type;
        }

        runtime_scope(op_tuple_t&& opt, base_t& base)
            : op_tuple(__fwd__(opt)) {
            base_scope = base;
        }

        ~runtime_scope() {
        }

    };

    template <runtime_scope_type scope_type,typename base_t, typename op_tuple_t>
    auto
    make_runtime_scope(base_t& scope, op_tuple_t&& opt) {
        return std::make_shared<
            runtime_scope<
                scope_type,
                op_tuple_t,
                __typ__(scope)
            >
        > (
            __fwd__(opt),
            scope
        );
    }

    template <uint32_t pos, uint32_t n, typename op_t, typename prev_t, typename T0, typename ...TS>
    auto
    change_tuple(op_t&& new_op, prev_t&& p, T0 &&t, TS&& ...ts) {
        if constexpr (pos == n) {
            return std::tuple_cat(__fwd__(p), std::forward_as_tuple(__fwd__(new_op)), std::forward_as_tuple(__fwd__(ts)...));
        } else {
            return change_tuple<pos + 1, n>(__fwd__(new_op), std::tuple_cat(__fwd__(p), std::forward_as_tuple(__fwd__(t))), __fwd__(ts)...);
        }
    }

    template <uint32_t pos, typename scope_t, typename new_op_t>
    auto*
    change_opt(scope_t& scope, new_op_t&& op) {
        return std::apply(
            [b = scope->base_scope, o = __fwd__(op)](auto&& ...args) mutable {
                return new runtime_scope<
                    scope_t::scope_type,
                    __typ__(b),
                    std::decay_t<decltype((change_tuple<0,pos,new_op_t,std::tuple<>, __typ__(args)...>(__fwd__(o), std::make_tuple(), __mov__(args)...)))>
                >(
                    b,
                    change_tuple<0,pos,new_op_t,std::tuple<>, __typ__(args)...>(__fwd__(o), std::make_tuple(), __mov__(args)...)
                );
            },
            __fwd__(scope->get_op_tuple())
        );
    }

    template <uint32_t pos, typename scope_t>
    struct
    get_current_op_type {
        using type = std::decay_t<std::tuple_element_t<pos, typename scope_t::element_type::op_tuple_type>>;
    };

    template <uint32_t pos, typename scope_t>
    requires std::is_same_v<
        std::decay_t<typename std::decay_t<std::tuple_element_t<pos, typename scope_t::element_type::op_tuple_type>>::op_type>,
        std::decay_t<typename std::decay_t<std::tuple_element_t<pos, typename scope_t::element_type::op_tuple_type>>::op_type>
    >
    struct
    get_current_op_type<pos, scope_t> {
        using type = std::decay_t<typename std::decay_t<std::tuple_element_t<pos, typename std::decay_t<scope_t>::element_type::op_tuple_type>>::op_type>;
    };
    template<uint32_t pos, typename scope_t>
    using get_current_op_type_t = get_current_op_type<pos, scope_t>::type;

    template <typename find_scope_t>
    inline
    auto&
    find_stream_starter(find_scope_t& scope){
        if constexpr (find_scope_t::element_type::scope_type == runtime_scope_type::stream_starter)
            return scope;
        else
            return find_stream_starter(scope->base_scope);
    }

    template<typename pop_scope_t>
    inline
    auto
    pop_to_loop_starter(pop_scope_t& scope) {
        if constexpr (pop_scope_t::element_type::scope_type == runtime_scope_type::root) {
            static_assert(pop_scope_t::element_type::scope_type != runtime_scope_type::root, "pop_to_loop_starter should not to root");
        }
        else if constexpr (pop_scope_t::element_type::scope_type == runtime_scope_type::stream_starter) {
            auto base = scope->base_scope;
            return base;
        }
        else {
            auto& base = scope->base_scope;
            return pop_to_loop_starter(base);
        }
    }
}

#endif //PUMP_SCOPE_HH
