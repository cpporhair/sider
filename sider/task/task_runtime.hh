//
//
//

#ifndef SIDER_DBMS_TASK_TASK_RUNTIME_HH
#define SIDER_DBMS_TASK_TASK_RUNTIME_HH

#include "sider/db/common/config.hh"
#include "./scheduler.hh"

namespace sider::task {
    struct
    runtime_scheduler_list {
        scheduler** by_core;
        std::vector<task::scheduler*> list;
        runtime_scheduler_list()
            : by_core(new task::scheduler* [std::thread::hardware_concurrency()]{nullptr}) {
        }
    };

    runtime_scheduler_list runtime_schedulers;
    bool running_flag;

    auto
    init_schedulers(const config::db_config& cfg) {
        uint64_t core_mask = cfg.compute_core_mask();

        for (uint32_t c = 0; c < std::thread::hardware_concurrency(); ++c) {
            if ((core_mask & (1 << c)) == 0) {
                runtime_schedulers.by_core[c] = nullptr;
            }
            else {
                runtime_schedulers.by_core[c] = new scheduler(c);
                runtime_schedulers.list.emplace_back(runtime_schedulers.by_core[c]);
            }
        }
    }
}

#endif //SIDER_DBMS_TASK_TASK_RUNTIME_HH
