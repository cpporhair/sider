//
// Created by null on 24-1-10.
//

#ifndef SIDER_NET_IO_URING_REQUEST_HH
#define SIDER_NET_IO_URING_REQUEST_HH

#include <liburing.h>

namespace sider::net::io_uring {
    enum struct
    uring_event_type {
        accept  = 0,
        read    = 1,
        write   = 2
    };

    struct
    io_uring_request {
        uring_event_type event_type;
        int iovec_count;
        int client_socket;
        void *user_data;
        struct iovec iov[];
    };
}

#endif //SIDER_NET_IO_URING_REQUEST_HH
