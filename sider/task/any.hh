//
//
//

#ifndef SIDER_DBMS_TASK_ANY_HH
#define SIDER_DBMS_TASK_ANY_HH

#include "sider/pump/on.hh"

#include "./scheduler.hh"

namespace sider::task {
    static
    auto
    schedule_at_any_task() {
        return pump::on(((scheduler*) nullptr)->get_scheduler());
    }

    static
    auto
    just_task() {
        return ((scheduler*) nullptr)->get_scheduler();
    }

    inline
    auto
    as_task() {
        return schedule_at_any_task();
    }

    static
    auto
    delay_at_any_task(uint64_t ms) {
        return pump::on(((scheduler*) nullptr)->delay(ms));
    }
}

#endif //SIDER_DBMS_TASK_ANY_HH
