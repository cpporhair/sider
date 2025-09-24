#ifndef PUMP_UTIL_META_HH
#define PUMP_UTIL_META_HH

#include <tuple>
#include <utility>
#include <variant>
#include <functional>

#define __raw__(x) x
#define __typ__(x) std::decay_t<std::remove_pointer_t<decltype(x)>>
#define __fwd__(x) std::forward<decltype(x)>(x)
//#define __fwd__(x) std::forward<std::decay_t<decltype(x)>>(x)
#define __mov__(x) std::move(x)
#define __ncp__(x) x : boost::noncopyable

#define __must_rval__(x) static_assert(meta::must_rvalue<decltype(x)>::v)
#define __all_must_rval__(x...) static_assert(meta::must_rvalue<decltype(x)...>::v)
#define __out_type_name__(x) typedef typename x::out_put_type_name out_put_type_name_##x

#define __forward_values__(x) \
if constexpr (sizeof...(x) > 1) return ::pump::tuple_values(__mov__(x)...);\
else if constexpr (sizeof...(x) == 1) return std::forward<std::tuple_element_t<0, std::tuple<decltype(x)...>>>(x...);\
else return ;

using uint08_t = uint8_t;

namespace pump::meta {
    template <auto N>
    using bool_ = std::bool_constant<N>;

    template <typename T>
    inline constexpr auto typev = T::value;

    template <typename T>
    using value_t = typename T::value_type;

    template <typename T>
    inline constexpr auto negav = std::negation_v<T>;

    template <typename T, typename = std::void_t<>>
    struct type_of {
        using type = T;
        static constexpr auto value = 0;
    };

    template <typename T>
    struct type_of<T, std::void_t<typename T::type>> {
        using type = typename T::type;
        static constexpr auto value = 1;
    };

    template <typename T>
    using type_of_t = typename type_of<T>::type;

    template <typename T>
    inline constexpr auto type_of_v = typev<type_of_t<T>>;

    template <bool B, typename T, typename U>
    using type_if = type_of_t<std::conditional_t<B, T, U>>;


    template <typename... Args>
    struct contains : std::false_type {
    };

    template <typename T, typename... Args>
    struct contains<T, Args...> : bool_<(std::is_same_v<T, Args> || ...)> {
    };

    template <typename ...>
    inline constexpr auto contains_v = std::false_type{};

    template <typename T, typename... Args>
    inline constexpr auto contains_v<T, Args...> = (std::is_same_v<T, Args> || ...);



    template <typename T>
    struct clear : std::type_identity<T> {
    };

    template <template <typename... > typename T, typename... Args>
    struct clear<T<Args...>> : std::type_identity<T<>> {
    };

    template <template <typename, auto ...> typename T, typename U, auto... Args>
    struct clear<T<U, Args...>> : std::type_identity<T<U>> {
    };

    template <typename T>
    using clear_t = type_of_t<clear<T>>;

    template <template <typename ...> typename F, typename T>
    struct unpack;

    template <template <typename ...> typename F, template <typename ...> typename T, typename... Args>
    struct unpack<F, T<Args...>> {
        using type = F<Args...>;
    };

    template <template <typename ...> typename F, typename T>
    using unpack_t = type_of_t<unpack<F, T>>;

    template <typename... Args>
    struct concat;

    template <template <typename ...> typename T, typename... Args>
    struct concat<T<Args...>> : std::type_identity<T<Args...>> {
    };

    template <template <typename, auto ...> typename T, typename U, auto... Args>
    struct concat<T<U, Args...>> : std::type_identity<T<U, Args...>> {
    };

    template <template <typename ...> typename T, typename... Args, typename... args>
    struct concat<T<Args...>, T<args...>> : std::type_identity<T<Args..., args...>> {
    };

    template <template <typename, auto ...> typename T, typename U, auto... Args, auto... args>
    struct concat<T<U, Args...>, T<U, args...>> : std::type_identity<T<U, Args..., args...>> {
    };

    template <template <typename ...> typename T, typename... Args, typename... args, typename... U>
    struct concat<T<Args...>, T<args...>, U...> : concat<T<Args..., args...>, U...> {
    };

    template <template <typename, auto ...> typename T, typename U, auto... Args, auto... args, typename... V>
    struct concat<T<U, Args...>, T<U, args...>, V...> : concat<T<U, Args..., args...>, V...> {
    };

    template <typename... Args>
    using concat_t = type_of_t<concat<Args...>>;



    template <typename T, typename U, typename V>
    struct dedup  {
        using uniq = std::conditional_t<negav<T>, U, clear_t<U>>;
        using type = concat_t<uniq, type_of_t<V>>;
    };

    template <typename T, typename U, typename V>
    using dedup_t = type_of_t<dedup<T, U, V>>;

    template <typename T>
    struct unique;

    template <template <typename ...> typename T, typename... Args>
    struct unique<T<Args...>> {
        template <typename U, typename... args>
        struct impl : std::type_identity<T<args...>> {
        };

        template <template <typename ...> typename U, typename... prev, typename V, typename... args>
        struct impl<U<prev...>, V, args...> {
            using type = dedup_t<contains<V, prev...>, U<V>, impl<U<prev..., V>, args...>>;
        };

        using type = type_of_t<impl<T<>, Args...>>;
    };

    template <template <typename, auto ...> typename T, typename U, auto... values>
    struct unique<T<U, values...>> : unpack<concat_t, type_of_t<unique<std::tuple<T<U, values>...>>>> {
    };

    template <typename T>
    using unique_t = type_of_t<unique<T>>;

    template<typename... Ts> struct is_tuple : std::false_type {};
    template<typename... Ts> struct is_tuple<std::tuple<Ts...>> : std::true_type {};
    template<typename... Ts> struct is_tuple<const std::tuple<Ts...>> : std::true_type {};
    template<typename... Ts> struct is_tuple<volatile std::tuple<Ts...>> : std::true_type {};
    template<typename... Ts> struct is_tuple<const volatile std::tuple<Ts...>> : std::true_type {};

    template <typename ...types_t>
    struct
    must_rvalue {
    };

    template <typename type_t>
    struct
    must_rvalue<type_t> {
        constexpr static bool v = std::is_rvalue_reference_v<type_t>;
    };

    template <typename type_t, typename ...types_t>
    struct
    must_rvalue<type_t,types_t...> {
        constexpr static bool v = std::is_rvalue_reference_v<type_t> && must_rvalue<types_t...>::v;
    };

    template <>
    struct
    must_rvalue<> {
        constexpr static bool v = true;
    };


    template <typename result_t, typename flag_name>
    struct
    check_flag {
        constexpr static bool value = false;
    };

    template<typename result_t, typename flag_name>
    requires std::is_same_v<bool, typename result_t::flag_name>
    struct
    check_flag<result_t, flag_name> {
        constexpr static bool value = result_t::flag_name;
    };

    template<typename result_t, typename flag_name>
    constexpr bool check_flag_v = check_flag<result_t, flag_name>::value;

    template<typename T>
    constexpr bool is_true = std::is_same_v<std::true_type ,T>;

    template<typename T>
    constexpr bool is_false = std::is_same_v<std::false_type ,T>;

    template <class T, class Tuple>
    struct has_type;

    template <class T, class... Us>
    struct has_type<T, std::tuple<Us...>> : std::disjunction<std::is_same<T, Us>...> {};
}

#endif


