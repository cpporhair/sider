//
// Created by null on 24-1-15.
//

#ifndef SIDER_NET_IO_URING_SEND_HH
#define SIDER_NET_IO_URING_SEND_HH

#include <memory>
#include "./runtime.hh"

namespace sider::net::io_uring {
    inline
    auto
    send(session_scheduler *scheduler, int socket, void *buf, uint32_t size) {
        return scheduler->send(socket, buf, size);
    }

    inline
    auto
    send(int socket, void* buf, uint32_t size) {
        return send(chose_session_scheduler(socket), socket, buf, size);
    }
}

#endif //SIDER_NET_IO_URING_SEND_HH
