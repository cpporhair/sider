//

//

#ifndef SIDER_KV_STOP_DB_HH
#define SIDER_KV_STOP_DB_HH

#include <filesystem>

namespace sider::kv {
    auto
    stop_db() {
        return pump::ignore_args()
            >> as_task(runtime::main_core)
            >> pump::then([](){
                runtime::running = false;
            });
    }
}

#endif //SIDER_KV_STOP_DB_HH
