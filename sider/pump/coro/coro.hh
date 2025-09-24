//
//
//

#ifndef SIDER_CORO_HH
#define SIDER_CORO_HH

#include <utility>
#include <optional>

#include "await.hh"
#include "object.hh"
#include "generator.hh"

namespace pump::coro {

    using empty_yields = coro::co_object<coro::yields_promise<std::default_sentinel_t>>;

    template <typename T>
    using return_yields = coro::co_object<coro::yields_promise<T>>;

    template <typename T>
    using maybe_yields = coro::co_object<coro::yields_promise<std::optional<T>>>;

    template <typename handle_owner_t>
    auto
    make_await_able(handle_owner_t&& o){
        return await_co_handle(__fwd__(o));
    }

    auto
    make_view_able(auto&& o){
        return co_view_able(__fwd__(o));
    }
}

#endif //SIDER_CORO_HH
