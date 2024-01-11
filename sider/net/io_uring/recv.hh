//
// Created by null on 24-1-11.
//

#ifndef SIDER_NET_IO_URING_RECV_HH
#define SIDER_NET_IO_URING_RECV_HH

#include "./session_scheduler.hh"

namespace sider::net::io_uring {
    auto
    recv(int socket, void* buf, uint32_t size) {
        return ((session_scheduler *) nullptr)->recv(socket, buf, size);
    }
}

#endif //SIDER_NET_IO_URING_RECV_HH
