//

//

#ifndef MONISM_YCSB_HH
#define MONISM_YCSB_HH

#include <iostream>
#include <ranges>
#include <any>

#include "sider/pump/just.hh"
#include "sider/pump/concurrent.hh"
#include "sider/pump/reduce.hh"
#include "sider/pump/sequential.hh"
#include "sider/pump/when_all.hh"
#include "sider/pump/coro/coro.hh"
#include "sider/kv/start_db.hh"
#include "sider/kv/start_batch.hh"
#include "sider/kv/put.hh"
#include "sider/kv/make_kv.hh"
#include "sider/kv/apply.hh"
#include "sider/kv/scan.hh"
#include "sider/kv/stop_db.hh"
#include "sider/kv/get.hh"

namespace ycsb {
    using namespace pump::coro;
    using namespace pump::meta;
    using namespace pump;
    using namespace sider;
    using namespace sider::kv;

    template<typename T>
    constexpr bool is_get = std::is_same_v<std::false_type ,T>;

    template<typename T>
    constexpr bool is_put = std::is_same_v<std::true_type ,T>;

    auto
    make_data(const char* seed, uint64_t size) {
        char* v = new char [1024 * 3];
        memset(v, '0', 1024 * 3);
        strcpy(v, seed);
        return v;
    }

    auto
    random_kv(uint64_t max) {
        return then([max](...){
            auto s = std::to_string(spdk_get_ticks() % max);
            auto k = make_data(s.c_str(), 64);
            auto v = make_data(s.c_str(), 1024 * 3);
            auto x = sider::kv::make_kv(k,v);
            delete k;
            delete v;
            return x;
        });
    }

    auto
    make_kv() {
        return then([](uint32_t i){
            auto s = std::to_string(i);
            auto k = make_data(s.c_str(), 64);
            auto v = make_data(s.c_str(), 1024 * 3);
            auto x = sider::kv::make_kv(k, v);
            delete k;
            delete v;
            return x;
        });
    }

    auto
    random_key(uint64_t max) {
        return then([max](...){
            auto k = make_data(std::to_string(spdk_get_ticks() % max).c_str(), 64);
            auto x = std::string(k);
            delete k;
            return x;
        });
    }

    inline auto
    load(uint64_t max) {
        return start_statistic()
            >> generate_on(any_task_scheduler(), std::views::iota(uint64_t(0), max))
            >> output_statistics_per_sec()
            >> concurrent(10000)
            >> as_batch(make_kv() >> put() >> apply() >> statistic_put()) >> statistic_publish()
            >> count()
            >> stop_statistic()
            >> output_finally_statistics();
    }

    inline auto
    updt(uint64_t max) {
        return start_statistic()
            >> generate_on(any_task_scheduler(), std::views::iota(uint64_t(0), max))
            >> output_statistics_per_sec()
            >> concurrent(10000)
            >> then([](uint64_t i) { return (spdk_get_ticks() % 100) < 50; })
            >> visit()
            >> then([max](auto &&res) {
                if constexpr (is_put<__typ__(res)>)
                    return as_batch( random_kv(max) >> put() >> apply() >> statistic_put());
                else
                    return as_batch(random_key(max) >> get() >> statistic_get());
            })
            >> then([](auto&& sender) {
                return just() >> sender;
            })
            >> flat()
            >> reduce()
            >> stop_statistic()
            >> output_finally_statistics();
    }

    inline auto
    read(uint64_t max) {
        return start_statistic()
            >> generate_on(any_task_scheduler(), std::views::iota(uint64_t(0), max))
            >> output_statistics_per_sec()
            >> concurrent(10000)
            >> as_batch(random_key(max) >> get() >> statistic_get())
            >> reduce()
            >> stop_statistic()
            >> output_finally_statistics();
    }

    inline auto
    scan(uint64_t max) {
        return start_statistic()
            >> generate_on(any_task_scheduler(), std::views::iota(uint64_t(0), max / 50))
            >> get_context<statistic_helper>()
            >> then([](statistic_helper &helper, ...) { helper.data->gen_count++; })
            >> output_statistics_per_sec()
            >> concurrent(10000)
            >> as_batch(
                random_key(max - 50)
                    >> start_scan()
                    >> repeat(50)
                    >> next()
                    >> then([](data::scan_result &&res) {
                        if (res.k == nullptr)
                            std::cout << "not found" << std::endl;
                    })
                    >> statistic_put()
                    >> reduce()
                    >> ignore_args()
            )
            >> count()
            >> stop_statistic()
            >> output_finally_statistics();
    }
}
#endif //MONISM_YCSB_HH
