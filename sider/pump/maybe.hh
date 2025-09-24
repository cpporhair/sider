//
//
//

#ifndef PUMP_MAYBE_HH
#define PUMP_MAYBE_HH

#include <utility>
#include <concepts>
#include <exception>
#include <optional>

#include "./bind_back.hh"

namespace pump {
    namespace _maybe {
        template <bool need_value>
        struct
        op {
            constexpr static bool maybe_op = true;
            constexpr static bool need_val = need_value;
        };

        template <bool has_value, typename prev_t>
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
                return op<has_value>{};
            }

            template<typename context_t>
            auto
            connect(){
                return prev.template connect<context_t>().push_back(make_op());
            }
        };

        struct
        maybe {
            template <typename sender_t>
            constexpr
            decltype(auto)
            operator ()(sender_t&& sender) const {
                return _maybe::sender<true, sender_t>{
                    __fwd__(sender)
                };
            }

            decltype(auto)
            operator ()() const {
                return bind_back<maybe>(maybe{});
            }
        };

        struct
        maybe_not {
            template <typename sender_t>
            constexpr
            decltype(auto)
            operator ()(sender_t&& sender) const {
                return _maybe::sender<false, sender_t>{
                    __fwd__(sender)
                };
            }

            decltype(auto)
            operator ()() const {
                return bind_back<maybe_not>(maybe_not{});
            }
        };
    }

    namespace pusher {
        template<uint32_t pos, typename scope_t>
        requires (pos < std::tuple_size_v<typename scope_t::element_type::op_tuple_type>)
        && (get_current_op_type_t<pos, scope_t>::maybe_op)
        struct
        op_pusher<pos, scope_t> : op_pusher_base<pos, scope_t> {

            template <typename context_t, typename value_t>
            requires get_current_op_type_t<pos, scope_t>::need_val
            static inline
            auto
            push_value(context_t& context, scope_t& scope, std::optional<value_t>&& v) {
                if(v.has_value())
                    op_pusher<pos + 1, scope_t>::push_value(context, scope, v.value());
                else
                    op_pusher<pos + 1, scope_t>::push_skip(context, scope);
            }

            template <typename context_t, typename value_t>
            static inline
            auto
            push_value(context_t& context, scope_t& scope, std::optional<value_t>&& v) {
                if(!v.has_value())
                    op_pusher<pos + 1, scope_t>::push_value(context, scope);
                else
                    op_pusher<pos + 1, scope_t>::push_skip(context, scope);
            }
        };
    }

    namespace typed {
        template <typename context_t, typename sender_t>
        struct
        compute_sender_type<context_t, pump::_maybe::sender<true, sender_t>> {
            using value_type    = compute_sender_type<context_t, sender_t>::value_type::value_type;
        };
    }

    inline constexpr _maybe::maybe maybe{};
    inline constexpr _maybe::maybe_not maybe_not{};
}
#endif //PUMP_MAYBE_HH
