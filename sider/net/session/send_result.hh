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
}

#endif //SIDER_NET_SESSION_SEND_RESULT_HH
