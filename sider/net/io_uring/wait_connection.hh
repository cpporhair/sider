//
// Created by null on 24-1-10.
//

#ifndef SIDER_NET_IO_URING_WAIT_CONNECTION_HH
#define SIDER_NET_IO_URING_WAIT_CONNECTION_HH

#include "./accept_scheduler.hh"

namespace sider::net::io_uring {
    inline
    auto
    wait_connection() {
        return ((accept_scheduler *) nullptr)->wait_connection();
    }
}

#endif //SIDER_NET_IO_URING_WAIT_CONNECTION_HH
