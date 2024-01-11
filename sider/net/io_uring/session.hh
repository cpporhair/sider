
#ifndef SIDER_NET_IO_URING_SESSION_HH
#define SIDER_NET_IO_URING_SESSION_HH

#include "spdk/memory.h"
#include "sider/util/ring.hh"

namespace sider::net::io_uring {
    struct
    session {
        int socket;
    };
}

#endif //SIDER_NET_IO_URING_SESSION_HH
