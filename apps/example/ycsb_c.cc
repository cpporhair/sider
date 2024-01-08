//

//

#include <iostream>
#include <ranges>
#include <any>

#include "sider/pump/just.hh"
#include "sider/pump/concurrent.hh"
#include "sider/pump/reduce.hh"
#include "sider/pump/sequential.hh"
#include "sider/coro/coro.hh"
#include "sider/kv/start_db.hh"
#include "sider/kv/start_batch.hh"
#include "sider/kv/get.hh"
#include "sider/kv/make_kv.hh"
#include "sider/kv/apply.hh"
#include "sider/kv/as_task.hh"
#include "sider/kv/stop_db.hh"
#include "sider/kv/put.hh"


#include "./statistic.hh"
#include "./ycsb.hh"

using namespace sider::pump;
using namespace sider::kv;
using namespace ycsb;

uint64_t max_key = 10000000;

int
main(int argc, char **argv){
    start_db(argc, argv)([](){
        return with_context(statistic_helper(new statistic_data()), logger())(
                start_statistic()
                    >> generate_on(any_task_scheduler(), std::views::iota(uint64_t(0), max_key))
                    >> output_statistics_per_sec()
                    >> concurrent(10000)
                    >> then([](uint64_t i) { return (spdk_get_ticks() % 100) < 50; })
                    >> visit()
                    >> then([](auto &&res) {
                        if constexpr (is_put<__typ__(res)>)
                            return just() >> as_batch( random_kv(max_key) >> put() >> apply() >> statistic_put());
                        else
                            return just() >> as_batch(random_key(max_key) >> get() >> statistic_get());
                    })
                    >> flat()
                    >> reduce()
                    >> stop_statistic()
                    >> output_finally_statistics()
            )
            >> stop_db()
            >> submit(make_root_context(logger()));
    });
    return 0;
}