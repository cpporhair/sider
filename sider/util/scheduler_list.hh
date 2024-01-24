//
// Created by null on 24-1-24.
//

#ifndef SIDER_UTIL_SCHEDULER_LIST_HH
#define SIDER_UTIL_SCHEDULER_LIST_HH

#include <thread>
#include <vector>

namespace sider::util {
    template <typename scheduler_t>
    struct
    scheduler_list {
        scheduler_t** by_core;
        std::vector<scheduler_t*> by_list;
        scheduler_list()
            : by_core(new scheduler_t* [std::thread::hardware_concurrency()]{nullptr}) {
        }
    };
}

#endif //SIDER_UTIL_SCHEDULER_LIST_HH
