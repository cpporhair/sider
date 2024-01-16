//

//

#include <iostream>
#include <ranges>
#include <any>

#include "sider/pump/just.hh"
#include "sider/pump/concurrent.hh"
#include "sider/pump/reduce.hh"
#include "sider/pump/sequential.hh"
#include "sider/pump/when_all.hh"
#include "sider/coro/coro.hh"
#include "sider/kv/start_db.hh"
#include "sider/kv/start_batch.hh"
#include "sider/kv/put.hh"
#include "sider/kv/make_kv.hh"
#include "sider/kv/apply.hh"
#include "sider/kv/scan.hh"
#include "sider/kv/stop_db.hh"
#include "sider/kv/get.hh"



#include "./statistic.hh"
#include "./ycsb.hh"

using namespace sider::coro;
using namespace sider::pump;
using namespace sider::db;
using namespace ycsb;


auto
ycsb_kv_coro(uint64_t max) -> return_yields<uint64_t> {
    for (uint64_t i = 0; i < max; ++i) {
        co_yield i;
    }
    co_return max;
}

uint64_t max_key = 10000000;

int
main(int argc, char **argv){
    start_db(argc, argv)([](){
        return with_context(statistic_helper(new statistic_data()), logger()) (load(max_key))
            >> with_context(statistic_helper(new statistic_data()), logger()) (updt(max_key))
            >> with_context(statistic_helper(new statistic_data()), logger()) (read(max_key))
            >> with_context(statistic_helper(new statistic_data()), logger()) (scan(max_key))
            >> stop_db();
    });
    return 0;
}