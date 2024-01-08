//

//

#ifndef SIDER_KV_AS_TASK_HH
#define SIDER_KV_AS_TASK_HH

#include "sider/task/any.hh"
#include "./runtime/scheduler_objects.hh"

namespace sider::kv {
    auto
    random_core() {
        return spdk_get_ticks() % runtime::task_schedulers.list.size();
    }

    auto
    random_task_scheduler() {
        return runtime::task_schedulers.list[random_core()]->get_scheduler();
    }

    auto
    as_task(uint32_t core = UINT32_MAX){
        return pump::then([core](auto&& ...args){
            if (core == UINT32_MAX)
                return random_task_scheduler()
                    >> pump::forward_value(__fwd__(args)...);
            else
                return runtime::task_schedulers.list[core]->get_scheduler()
                    >> pump::forward_value(__fwd__(args)...);
        })
        >> pump::flat();
    }

    auto
    any_task_scheduler(uint32_t core = UINT32_MAX){
        return pump::then([core](auto&& ...args){
            if (core == UINT32_MAX)
                return random_task_scheduler()
                    >> pump::forward_value(__fwd__(args)...);
            else
                return runtime::task_schedulers.list[core]->get_scheduler()
                    >> pump::forward_value(__fwd__(args)...);
        })
            >> pump::flat();
    }

    template<typename bind_back_t, std::ranges::viewable_range range_t>
    auto
    generate_on(bind_back_t&& t, range_t&& r){
        return pump::generate(__fwd__(r)) >> __fwd__(t);
    }

    template<typename bind_back_t>
    auto
    generate_on(bind_back_t&& t){
        return [t = __fwd__(t)](auto&& r) mutable { return pump::generate(__fwd__(r)) >> __fwd__(t); };
    }
}

#endif //SIDER_KV_AS_TASK_HH
