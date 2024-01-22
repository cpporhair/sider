//
// Created by null on 24-1-15.
//

#ifndef SIDER_NET_SESSION_SEND_RESULT_HH
#define SIDER_NET_SESSION_SEND_RESULT_HH

#include "sider/pump/then.hh"

#include "sider/net/io_uring/send.hh"

namespace sider::net::session {
    inline
    auto
    send_res() {
        return pump::ignore_args();
    }

    inline
    auto
    send_put_res(put_res&& res) {
        return pump::get_context<session>()
            >> pump::then([res = __fwd__(res)](session &s) {
                return io_uring::send(s.socket, nullptr, 0);
            })
            >> pump::flat();
    }

    inline
    auto
    send_get_res() {
    }
}

#endif //SIDER_NET_SESSION_SEND_RESULT_HH
