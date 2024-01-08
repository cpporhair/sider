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
#include "sider/kv/put.hh"
#include "sider/kv/make_kv.hh"
#include "sider/kv/apply.hh"
#include "sider/kv/scan.hh"
#include "sider/kv/stop_db.hh"


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
                    >> as_batch(make_kv() >> put() >> apply() >> statistic_put()) >> statistic_publish()
                    >> count()
                    >> stop_statistic()
                    >> output_finally_statistics()
            )
            >> with_context(statistic_helper(new statistic_data()), logger())(
                start_statistic()
                    >> generate_on(any_task_scheduler(), std::views::iota(0, 10000000 / 50))
                    >> get_context<statistic_helper>()
                    >> then([](statistic_helper &helper, ...) { helper.data->gen_count++; })
                    >> output_statistics_per_sec()
                    >> concurrent(10000)
                    >> as_batch([]() {
                        return just()
                            >> random_key(10000000 - 50)
                            >> start_scan()
                            >> repeat(50)
                            >> next()
                            >> then([](data::scan_result &&res) {
                                if (res.k == nullptr)
                                    std::cout << "not found" << std::endl;
                            })
                            >> statistic_put()
                            >> reduce()
                            >> ignore_args();
                    })
                    >> count()
                    >> stop_statistic()
                    >> output_finally_statistics()
            )
            >> stop_db();
    });
    return 0;
}