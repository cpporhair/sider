//
// Created by null on 24-1-10.
//

#ifndef SIDER_NET_IO_URING_WAIT_CONNECTION_HH
#define SIDER_NET_IO_URING_WAIT_CONNECTION_HH

#include "./runtime.hh"

namespace sider::net::io_uring {
    inline
    auto
    wait_connection() {
        return accept_schedulers.by_list[0]->wait_connection();
    }
}

#endif //SIDER_NET_IO_URING_WAIT_CONNECTION_HH
