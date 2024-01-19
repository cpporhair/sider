//
// Created by null on 24-1-18.
//

#ifndef SIDER_NET_SESSION_UNTIL_SESSION_CLOSED_HH
#define SIDER_NET_SESSION_UNTIL_SESSION_CLOSED_HH

#include "sider/coro/coro.hh"
#include "sider/pump/get_context.hh"
#include "sider/pump/push_context.hh"
#include "sider/pump/pop_context.hh"
#include "sider/pump/reduce.hh"
#include "sider/pump/then.hh"
#include "sider/pump/for_each.hh"

#include "./session.hh"

namespace sider::net::session {
    auto
    until_session_closed_coro(session& s) -> coro::empty_yields {
        while (s.is_lived())
            co_yield {};
        co_return {};
    }

    inline
    auto
    until_session_closed() {
        return pump::get_context<session>()
            >> pump::then([](session &s) { return make_view_able(until_session_closed_coro(s)); })
            >> pump::for_each()
            >> pump::ignore_results();
    }

    inline
    auto
    until_session_closed(session &&s){
        return [s = __fwd__(s)](auto&& b) mutable {
            return pump::with_context(__fwd__(s))(
                until_session_closed()
                    >> __fwd__(b)
                    >> pump::reduce()
            );
        };
    };
}

#endif //SIDER_NET_SESSION_UNTIL_SESSION_CLOSED_HH
