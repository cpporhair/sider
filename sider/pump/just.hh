//
//
//

#ifndef PUMP_JUST_HH
#define PUMP_JUST_HH

#include "./op_pusher.hh"
#include "./compute_sender_type.hh"
#include "./tuple_values.hh"
#include "./op_tuple_builder.hh"

namespace pump {
    namespace _just {
        template <typename ...value_t>
        struct
        __ncp__(op) {

            constexpr static bool just_op = true;
            constexpr static bool empty = (sizeof...(value_t) == 0);
            constexpr static bool is_exception() {
                if constexpr (sizeof...(value_t) != 1)
                    return false;
                else
                    return std::is_same_v<std::exception_ptr, std::tuple_element_t<0, std::tuple<value_t...>>>;
            }

            tuple_values<value_t...> values;

            op(tuple_values<value_t...>&& v)
                : values(__fwd__(v)){
            }

            op(op&& o)noexcept
                : values(__fwd__(o.values)){
            }

            op(const op& o)noexcept
                : values(o.values){
            }
        };

        template <typename ...value_t>
        struct
        __ncp__(sender) {
            constexpr static bool start_sender = true;
            tuple_values<value_t...> values;
            explicit
            sender(value_t&& ...v)
                :values(__fwd__(v)...){
            }

            sender(sender&& o)
                :values(__fwd__(o.values)){
            }

            inline
            auto
            make_op() {
                return op<value_t...>(__mov__(values));
            }

            template<typename context_t>
            auto
            connect(){
                return builder::op_list_builder<0>().push_back(make_op());
            }
        };

        struct
        fn {
            template <typename ...arg_t>
            constexpr
            decltype(auto)
            operator ()(arg_t&& ...arg) const {
                __all_must_rval__(arg);
                return _just::sender<__typ__(arg)...>{
                    __fwd__(arg)...
                };
            }

            template <typename arg_t>
            constexpr
            decltype(auto)
            operator ()(arg_t* arg) const {
                return _just::sender<arg_t*>{
                    __fwd__(arg)
                };
            }

            decltype(auto)
            operator ()(void) const {
                return _just::sender<>{
                };
            }
        };
    }

    namespace pusher {
        template<uint32_t pos, typename scope_t>
        requires (pos < std::tuple_size_v<typename scope_t::element_type::op_tuple_type>)
        && (get_current_op_type_t<pos, scope_t>::just_op)
        struct
        op_pusher<pos, scope_t> : op_pusher_base<pos, scope_t> {

            template<typename context_t>
            static inline
            void
            push_value(context_t& context, scope_t& scope)
            requires (get_current_op_type_t<pos, scope_t>::empty) {
                op_pusher<pos + 1, scope_t>::push_value(context, scope);
            }

            template<typename context_t>
            static inline
            void
            push_value(context_t& context, scope_t& scope)
            requires (get_current_op_type_t<pos, scope_t>::is_exception()) {
                std::apply(
                    [&context, &scope](auto&& ...args) mutable {
                        op_pusher<pos + 1, scope_t>::push_exception(context, scope , __fwd__(args)...);
                    },
                    std::get<pos>(scope->get_op_tuple()).values.values
                );
            }

            template<typename context_t>
            static inline
            void
            push_value(context_t& context, scope_t& scope) {
                std::apply(
                    [&context, &scope](auto&& ...args) mutable {
                        op_pusher<pos + 1, scope_t>::push_value(context, scope , __fwd__(args)...);
                    },
                    std::get<pos>(scope->get_op_tuple()).values.values
                );
            }
        };
    }

    namespace typed {
        template <typename context_t, typename value_t>
        struct
        compute_sender_type<context_t, pump::_just::sender<value_t>> {
            using value_type = value_t;
            constexpr static bool has_value_type = true;
        };

        template <typename context_t>
        struct
        compute_sender_type<context_t, pump::_just::sender<>> {
            constexpr static bool has_value_type = false;
        };

        template <typename context_t, typename value_t>
        struct
        compute_context_type<context_t, pump::_just::sender<value_t>> {
            using type = context_t;
        };

        template <typename context_t>
        struct
        compute_context_type<context_t, pump::_just::sender<>> {
            using type = context_t;
        };
    }

    inline constexpr _just::fn just{};
}
#endif //PUMP_JUST_HH
