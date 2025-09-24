//
//
//

#ifndef PUMP_OP_PUSHER_HH
#define PUMP_OP_PUSHER_HH

#include "./any_context.hh"
#include "./op_tuple_builder.hh"

namespace pump {

    template <typename ...op_t>
    using op_list_type = std::unique_ptr<std::tuple<op_t...>>;
    namespace pusher {

        constexpr uint64_t stream_push_first = 0;
        constexpr uint64_t stream_push_next = 1;

        constexpr bool has_flag(const uint64_t v, const uint64_t flag) {
            return (v | flag) != 0;
        }

        constexpr bool is_stream_push_next(const uint64_t v) {
            return has_flag(v, stream_push_next);
        }

        constexpr uint32_t flag_get_pos(const uint64_t v) {
            return uint32_t (v & UINT32_MAX);
        }

        constexpr uint32_t flag_next(const uint64_t v) {
            return uint32_t (v + 1);
        }

        constexpr uint32_t flag_prev(const uint64_t v) {
            return uint32_t (v + 1);
        }


        template<uint32_t pos, typename scope_t>
        struct
        op_pusher {
        };

        template<uint32_t pos, typename scope_t>
        struct
        op_pusher_base {
            constexpr static
            uint64_t
            compute_current_op_scope_id() {
                return uint64_t(scope_t::get_scope_level_id()) << 32 | uint64_t(pos);
            }
            template<typename context_t, typename ...value_t>
            static inline
            void
            push_value(context_t& context, scope_t& scope, value_t&& ...v) {
                op_pusher<pos + 1, scope_t>::push_value(context, scope, __fwd__(v)...);
            }

            template <typename context_t>
            static inline
            void
            push_exception(context_t& context, scope_t& scope, std::exception_ptr e) {
                op_pusher<pos + 1, scope_t>::push_exception(context, scope, e);
            }

            template <typename context_t>
            static inline
            void
            push_skip(context_t& context, scope_t& scope) {
                op_pusher<pos + 1, scope_t>::push_skip(context, scope);
            }

            template <typename context_t>
            static inline
            void
            push_done(context_t& context, scope_t& scope) {
                op_pusher<pos + 1, scope_t>::push_done(context, scope);
            }

            template <typename context_t>
            static inline
            void
            poll_next(context_t& context, scope_t& scope) {
                op_pusher<pos - 1, scope_t>::poll_next(context, scope);
            }
        };


        template <uint32_t pos>
        struct
        pop_pusher_scope_op {
            constexpr static bool pop_pusher_scope_op_flag = true;
            constexpr static uint32_t base_scope_runner_pos = pos;
        };

        template<uint32_t pos, typename scope_t>
        requires (pos < std::tuple_size_v<typename scope_t::element_type::op_tuple_type>)
        && (get_current_op_type_t<pos, scope_t>::pop_pusher_scope_op_flag)
        struct
        op_pusher<pos, scope_t> : op_pusher_base<pos, scope_t>  {
            constexpr static uint32_t new_pos = std::tuple_element_t<pos, typename scope_t::element_type::op_tuple_type>::base_scope_runner_pos;

            template<typename context_t, typename ...value_t>
            static inline
            void
            push_value(context_t& context, scope_t& scope, value_t&& ...v) {
                auto base_scope = scope->base_scope;
                op_pusher<new_pos, __typ__(base_scope)>::push_value(context, base_scope, __fwd__(v)...);
            }

            template<typename context_t>
            static inline
            void
            push_exception(context_t& context, scope_t& scope, std::exception_ptr e) {
                auto base_scope = scope->base_scope;
                op_pusher<new_pos, __typ__(base_scope)>::push_exception(context, base_scope, e);
            }

            template<typename context_t>
            static inline
            void
            push_skip(context_t& context, scope_t& scope) {
                auto base_scope = scope->base_scope;
                op_pusher<new_pos, __typ__(base_scope)>::push_skip(context, base_scope);
            }

            template<typename context_t>
            static inline
            void
            push_done(context_t& context, scope_t& scope) {
                auto base_scope = scope->base_scope;
                op_pusher<new_pos, __typ__(base_scope)>::push_done(context, base_scope);
            }
        };
    }}

#endif //PUMP_OP_PUSHER_HH
