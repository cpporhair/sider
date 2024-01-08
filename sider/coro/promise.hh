#ifndef SIDER_PROMISE_HH
#define SIDER_PROMISE_HH

#include <boost/noncopyable.hpp>
#include "base.hh"

namespace sider::coro {
    template <typename value_type>
    struct
    return_promise : boost::noncopyable , promise_base<value_type> {
    public:
        using handle_type           = std::coroutine_handle<return_promise<value_type>>;
    public:

        void
        return_value(value_type&& v) noexcept {
            this->value.reset(new std::decay_t<value_type>(__fwd__(v)));
        }

        void
        return_value(value_type& v) noexcept {
            this->value.reset(new std::decay_t<value_type>(__fwd__(v)));
        }

        auto
        get_return_object(){
            return co_object<return_promise>(handle_type::from_promise(*this));
        }
    };

    template <typename T>
    struct
    co_value<coro::co_object<coro::return_promise<T>>>{
        using type = T;
    };

    template <typename value_type>
    struct
    yields_promise : boost::noncopyable , promise_base<value_type> {
        using yield                 = value_type;
        using handle_type           = std::coroutine_handle<yields_promise<value_type>>;
    public:
        yields_promise() = default;

        void
        return_value(value_type&& v) noexcept {
            this->value.reset(new std::decay_t<value_type>(__fwd__(v)));
        }

        std::suspend_always
        yield_value(value_type&& v) noexcept {
            this->value.reset(new std::decay_t<value_type>(__fwd__(v)));
            return {};
        }

        std::suspend_always
        yield_value(value_type& v) noexcept {
            this->value.reset(new std::decay_t<value_type>(__fwd__(v)));
            return {};
        }

        auto
        get_return_object(){
            return co_object<yields_promise>(handle_type::from_promise(*this));
        }
    };

    template <typename T>
    struct
    co_value<coro::co_object<coro::yields_promise<T>>>{
        using type = T;
    };
}
#endif //SIDER_PROMISE_HH
