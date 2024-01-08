//
//
//

#ifndef SIDER_PUMP_TEST_RECEIVER_HH
#define SIDER_PUMP_TEST_RECEIVER_HH

#include "./op_pusher.hh"

namespace sider::pump {
    struct
    null_receiver {
        constexpr static bool null_receiver_op = true;
    };

    namespace pusher {

        template<uint32_t pos, typename scope_t>
        requires (pos < std::tuple_size_v<typename scope_t::element_type::op_tuple_type>)
        && (get_current_op_type_t<pos, scope_t>::null_receiver_op)
        struct
        op_pusher<pos, scope_t> : op_pusher_base<pos, scope_t> {
            template <typename context_t, typename ...value_t>
            static inline
            void
            push_value(context_t& context, scope_t& scope, value_t&& ...v) {
                static_assert(context_t::element_type::root_flag,"context is not root when op done");
            }

            template <typename context_t>
            static inline
            void
            push_exception(context_t& context, scope_t& scope, std::exception_ptr e) {
                static_assert(context_t::element_type::root_flag,"context is not root when op done");
            }

            template <typename context_t>
            static inline
            void
            push_skip(context_t& context, scope_t& scope) {
                static_assert(context_t::element_type::root_flag,"context is not root when op done");
            }

            template <typename context_t>
            inline
            void
            push_done(context_t& context, scope_t& scope) {
                static_assert(context_t::element_type::root_flag,"context is not root when op done");
            }
        };


    }

    inline constexpr null_receiver the_null_receiver{};
}

#endif //SIDER_PUMP_TEST_RECEIVER_HH
