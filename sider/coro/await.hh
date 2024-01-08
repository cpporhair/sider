//
//
//

#ifndef SIDER_AWAIT_HH
#define SIDER_AWAIT_HH

#include <memory>
#include <coroutine>
#include <boost/noncopyable.hpp>

namespace sider::coro {
    template <typename handle_type>
    struct
    await_co_handle : boost::noncopyable {
        handle_type handle;

        await_co_handle
        (await_co_handle&& rhs) noexcept
        :handle(rhs.handle){
        }
        explicit
        await_co_handle
        (handle_type h)
        :handle(h){
        }

        bool
        await_ready(){
            return handle.done();
        }
        auto
        await_resume() {
            handle.resume();
            return __mov__(handle.promise().take());
        }
        bool
        await_suspend(const std::coroutine_handle<>& h){
            return false;
        }
    };
}

#endif //SIDER_AWAIT_HH
