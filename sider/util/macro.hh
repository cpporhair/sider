#pragma clang diagnostic push
#pragma ide diagnostic ignored "bugprone-reserved-identifier"
//
//
//

#ifndef SIDER_UTIL_MACRO_HH
#define SIDER_UTIL_MACRO_HH

#include <memory>
#include <boost/noncopyable.hpp>
#include "./meta.hh"

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
if constexpr (sizeof...(x) > 1) return sider::pump::tuple_values(__mov__(x)...);\
else if constexpr (sizeof...(x) == 1) return std::forward<std::tuple_element_t<0, std::tuple<decltype(x)...>>>(x...);\
else return ;

using uint08_t = uint8_t;

#endif //SIDER_UTIL_MACRO_HH

#pragma clang diagnostic pop