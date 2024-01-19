//
// Created by null on 24-1-15.
//

#ifndef SIDER_NET_SESSION_SEND_RESULT_HH
#define SIDER_NET_SESSION_SEND_RESULT_HH

#include "sider/pump/then.hh"

namespace sider::net::session {
    inline
    auto
    send_res() {
        return pump::ignore_args();
    }

    inline
    auto
    send_put_res() {
    }

    inline
    auto
    send_get_res() {
    }
}

#endif //SIDER_NET_SESSION_SEND_RESULT_HH
