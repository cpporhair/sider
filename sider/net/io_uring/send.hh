//
// Created by null on 24-1-15.
//

#ifndef SIDER_NET_IO_URING_SEND_HH
#define SIDER_NET_IO_URING_SEND_HH

#include <memory>
#include "./session_scheduler.hh"

namespace sider::net::io_uring {
    inline
    auto
    send(int socket, void* buf, uint32_t size) {
        return ((session_scheduler *) nullptr)->send(socket, buf, size);
    }
}

#endif //SIDER_NET_IO_URING_SEND_HH
