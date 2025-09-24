//
// Created by null on 24-1-11.
//

#ifndef SIDER_NET_IO_URING_RECV_HH
#define SIDER_NET_IO_URING_RECV_HH

#include "./session_scheduler.hh"

namespace sider::net::io_uring {

    inline
    auto
    recv(session_scheduler *scheduler, int socket, void *buf, uint32_t size){
        return scheduler->recv(socket, buf, size);
    }

    inline
    auto
    recv(int socket, void* buf, uint32_t size) {
        return recv(chose_session_scheduler(socket), socket, buf, size);
    }
}

#endif //SIDER_NET_IO_URING_RECV_HH
