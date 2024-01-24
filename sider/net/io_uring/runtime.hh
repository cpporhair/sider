//
// Created by null on 24-1-24.
//

#ifndef SIDER_NET_IO_URING_RUNTIME_HH
#define SIDER_NET_IO_URING_RUNTIME_HH

#include <thread>
#include "sider/util/scheduler_list.hh"
#include "./accept_scheduler.hh"
#include "./session_scheduler.hh"

namespace sider::net::io_uring {
    util::scheduler_list<accept_scheduler>  accept_schedulers;
    util::scheduler_list<session_scheduler> session_schedulers;
}

#endif //SIDER_NET_IO_URING_RUNTIME_HH
