//
//
//

#ifndef SIDER_PUMP_COMPUTE_SENDER_TYPE_HH
#define SIDER_PUMP_COMPUTE_SENDER_TYPE_HH

#include "sider/util/meta.hh"

namespace sider::pump::typed {
    template <typename context_t, typename sender_t>
    struct
    compute_context_type {
        using type = compute_context_type<context_t, typename sender_t::prev_type>::type;
    };

    template <typename C, typename T>
    struct
    compute_sender_type {
    };

    template <typename ...T>
    struct
    tuple_push_front{};

    template <typename A0, typename ...A1>
    struct
    tuple_push_front<A0,std::tuple<A1...>> {
        using type = std::tuple<A0,A1...>;
    };

    template <typename T>
    requires std::is_same_v<decltype(T::has_value_type),decltype(T::has_value_type)>
    constexpr bool
    _has_value_type() {
        return T::has_value_type;
    }

    template <typename T>
    requires (!std::is_same_v<decltype(T::has_value_type),decltype(T::has_value_type)>)
    && std::is_same_v<typename T::value_type, typename T::value_type>
    constexpr bool
    _has_value_type() {
        return !std::is_void_v<typename T::value_type>;
    }

    template <typename T>
    constexpr bool
    _has_value_type() {
        return false;
    }

    template <typename T>
    constexpr bool has_value_type = _has_value_type<T>();


    template <typename T>
    concept mul_value_type = T::multi_values;

    struct
    apply {
        template <typename F, typename T>
        constexpr
        decltype(auto)
        operator()(F&& f,T&& t){
            return std::apply(
                [f = __fwd__(f)](auto&& ...args) mutable {
                    return f(__fwd__(args)...);
                },
                __mov__(t)
            );
        }
    };
}

#endif //SIDER_COMPUTE_SENDER_TYPE_HH
