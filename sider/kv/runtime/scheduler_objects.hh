//

//

#ifndef MONISM_SCHEDULER_OBJECTS_HH
#define MONISM_SCHEDULER_OBJECTS_HH

#include "sider/task/scheduler.hh"
#include "sider/kv/batch/scheduler.hh"
#include "sider/kv/index/scheduler.hh"
#include "sider/kv/fs/scheduler.hh"
#include "sider/kv/nvme/scheduler.hh"

#include "./config.hh"

namespace sider::kv::runtime {
    struct
    task_scheduler_list {
        task::scheduler** by_core;
        std::vector<task::scheduler*> list;
        task_scheduler_list()
            : by_core(new task::scheduler* [std::thread::hardware_concurrency()]{nullptr}) {
        }
    };

    task_scheduler_list task_schedulers;

    struct
    batch_scheduler_list {
        batch::scheduler** by_core;
        std::vector<batch::scheduler*> list;
        batch_scheduler_list()
            : by_core(new batch::scheduler* [std::thread::hardware_concurrency()]{nullptr}) {
        }
    };

    batch_scheduler_list batch_schedulers;

    struct
    index_scheduler_list {
        index::scheduler** by_core;
        std::vector<index::scheduler*> list;
        index_scheduler_list()
            : by_core(new index::scheduler* [std::thread::hardware_concurrency()]{nullptr}) {
        }
    };

    index_scheduler_list index_schedulers;

    struct
    fs_scheduler_list {
        fs::scheduler** by_core;
        std::vector<fs::scheduler*> list;
        fs_scheduler_list()
            : by_core(new fs::scheduler* [std::thread::hardware_concurrency()]{nullptr}) {
        }
    };

    fs_scheduler_list fs_schedulers;


    struct
    nvme_scheduler_list {
        const char* ssd_sn;
        nvme::scheduler** by_core;
        std::vector<nvme::scheduler*> list;
        nvme_scheduler_list(const char* sn)
            : ssd_sn(sn)
            , by_core(new nvme::scheduler* [std::thread::hardware_concurrency()]{nullptr}) {
        }
    };

    std::vector<nvme_scheduler_list> nvme_schedulers_per_ssd;
}

#endif //MONISM_SCHEDULER_OBJECTS_HH
