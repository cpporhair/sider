//

//

#ifndef MONISM_YCSB_CORO_HH
#define MONISM_YCSB_CORO_HH

#include <utility>
#include <memory>
#include <ranges>

#include "pump/any_context.hh"
#include "pump/any_exception.hh"
#include "pump/flat.hh"
#include "pump/null_receiver.hh"
#include "pump/get_context.hh"
#include "pump/push_context.hh"
#include "pump/pop_context.hh"
#include "pump/just.hh"
#include "pump/any_context.hh"
#include "pump/for_each.hh"
#include "pump/test_schedule.hh"
#include "pump/when_skipped.hh"
#include "pump/then.hh"
#include "pump/maybe.hh"
#include "pump/when_all.hh"
#include "pump/flat.hh"
#include "pump/reduce.hh"
#include "pump/repeat.hh"
#include "pump/range_adaptor.hh"

#include "sider/db/kv.hh"
#include "sider/db/put.hh"
#include "sider/db/get.hh"
#include "sider/db/apply.hh"
#include "sider/db/finish_batch.hh"
#include "sider/db/submit.hh"
#include "sider/db/start_batch.hh"
#include "sider/db/as_batch.hh"
#include "sider/db/scan.hh"
#include "sider/task/task.hh"

#include "statistic.hh"
#include "ycsb.hh"

using namespace sider::db;
using namespace sider::pump;
using namespace sider::task;
using namespace ycsb;

namespace ycsb::coro {

    uint64_t gen_count[1024] = {0};

    std::atomic<uint64_t> all_keys(1);

    auto
    load_one(uint64_t k) {
        just()
            >> with_read_write_batch(
                put_to_batch(make_kv(k))
                >> then([](...){})
                >> apply()
                >> statistic_put(4)
            )
            >> statistic_publish()
            >> submit_with_context(statistic_helper{&ycsb::g_statistic_data}, logger{k}, stopper{false});
    }

    auto
    read_one(uint64_t k) {
        just()
            >> with_readonly_batch(get("123") >> statistic_put())
            >> submit_with_context(statistic_helper{&ycsb::g_statistic_data}, logger{0}, stopper{false});
    }

    auto
    make_ycsb_put_list() {
        return
            std::views::iota(uint64_t(1),ycsb::g_ycsb_options.key_range)
            | std::views::transform([](uint64_t k){
                return make_kv(k);
            });
    }

    auto
    load()
    ->sider::coro::return_yields<sider::kv::opt_exit_coro_flag> {
        just()
            >> as_task(task_any(), generate(make_ycsb_put_list()))
            >> concurrent()
            >> with_read_write_batch(
                put_to_batch()
                    >> apply()
                    >> statistic_put()
            )
            >> statistic_publish()
            >> reduce()
            >> submit_with_context(statistic_helper{&ycsb::g_statistic_data}, logger{0}, stopper{false});

        while(all_keys < ycsb::g_ycsb_options.key_range) {
            if (ycsb::g_statistic_data.gen_count - ycsb::g_statistic_data.put_count < 300000)
                load_one(all_keys.fetch_add(1));
            //read_one(all_keys.fetch_add(1));
            co_yield sider::kv::opt_exit_coro_flag();
        }
        co_return {};
    }

    auto
    updt()
    ->sider::coro::empty_yields {
        co_return {};
    }

    auto
    read()
    ->sider::coro::empty_yields {
        co_return {};
    }

    auto
    scan()
    ->sider::coro::empty_yields {

        /*
        when_all(
                just()
                    >> with_readonly_batch(
                        start_scan("1")
                            >> scan_next([](uint64_t u){})
                            >> scan_next([](uint64_t u){})
                            >> scan_next([](uint64_t u){})
                    ),
                just(make_kv(1))
                    >> with_read_write_batch(
                        put_to_batch()
                            >> apply()
                            >> statistic_put()
                    )
            )
            >> then([](auto&& a1,auto&& a2) {

            })
            >> submit_with_context();
        //start_generator(scan()) >>
        just()
            >> with_readonly_batch(
                start_scan("1")
                    >> scan_next([](uint64_t u){})
                    >> scan_next([](uint64_t u){})
                    >> scan_next([](uint64_t u){})
            )
            >> with_readonly_batch(
                start_scan("1")
                    >> scan_next([](uint64_t u){})
                    >> scan_next([](uint64_t u){})
                    >> scan_next([](uint64_t u){})
            )
            >> submit_with_context();

        just_batch()
            >> start_scan("1")
            >> scan_next([](uint64_t u){})
            >> scan_next([](uint64_t u){})
            >> scan_next([](uint64_t u){})
            >> release()
            >> submit_with_context();
        just_batch()
            >> start_scan("1")
            >> until(
                scan_next([](uint64_t){return "123";})
                    >> get()
                    >> put_to_batch()
                    >> statistic_alc(1)
                    >> when_skipped([](){return just(true);})
            )
            >> apply()
            >> publish()
            >> submit_with_context();
        */
        co_return {};
    }

#include "sider/nvme/scheduler.hh"

    auto
    info()
    ->sider::coro::return_yields<sider::kv::opt_exit_coro_flag>{
        while (ycsb::g_statistic_data.put_count <= 0 && ycsb::g_statistic_data.get_count)
            co_yield sider::kv::opt_exit_coro_flag();
        auto n0 = std::chrono::steady_clock::now();
        auto n1 = std::chrono::steady_clock::now();
        auto last_tick = uint64_t(ycsb::g_ycsb_options.duration) * 1000 * 1000 * 1000;
        auto one_second = 1000 * 1000 * 1000;
        g_statistic_data.start_tick.store(spdk_get_ticks()) ;
        for(;;) {
            auto taked1 = std::chrono::steady_clock::now() - n0;
            if(taked1.count() >= last_tick)
                break;
            auto taked2 = std::chrono::steady_clock::now() - n1;
            if(taked2.count() >= one_second) {
                n1 = std::chrono::steady_clock::now();
                std::cout << std::endl;
                std::cout << "put_to_batch : " << ycsb::g_statistic_data.put_count << std::endl;
                std::cout << "get : " << ycsb::g_statistic_data.get_count << std::endl;
                std::cout << "pub : " << ycsb::g_statistic_data.pub_count << std::endl;
                std::cout << "gen : " << ycsb::g_statistic_data.gen_count << std::endl;
                std::cout << "sqn : " << sider::batch::detail::waited_sqno << std::endl;
                uint64_t alc = 0, enq = 0, deq = 0;
                for (int i = 0; i < 1024; ++i) {
                    alc += sider::nvme::detail::alc_count[i];
                    enq += sider::nvme::detail::enq_count[i];
                    deq += sider::nvme::detail::deq_count[i];
                }
                std::cout << "alc : " << alc << std::endl;
                std::cout << "enq : " << enq << std::endl;
                std::cout << "deq : " << deq << std::endl;

                for (int i = 0; i < 1024; ++i) {
                    if (sider::nvme::detail::tic_count[i] > 0)
                        std::cout << "tic[" << i << "] :" << sider::nvme::detail::tic_count[i] << std::endl;
                    if (sider::nvme::detail::alc_count[i] > 0)
                        std::cout << "    alc[" << i << "] :" << sider::nvme::detail::alc_count[i] << std::endl;

                    if (gen_count[i] > 0)
                        std::cout << "    gen[" << i << "] :" << gen_count[i] << std::endl;
                    if (sider::nvme::detail::enq_count[i] > 0)
                        std::cout << "    enq[" << i << "] :" << sider::nvme::detail::enq_count[i] << std::endl;
                    if (sider::nvme::detail::deq_count[i] > 0)
                        std::cout << "    deq[" << i << "] :" << sider::nvme::detail::deq_count[i] << std::endl;
                    if (sider::nvme::detail::qpr_count[i] > 0)
                        std::cout << "    qpr[" << i << "] :" << sider::nvme::detail::qpr_count[i] << std::endl;
                    if (sider::index::ieq_count[i] > 0)
                        std::cout << "    ieq[" << i << "] :" << sider::index::ieq_count[i] << std::endl;
                    if (sider::index::idq_count[i] > 0)
                        std::cout << "    idq[" << i << "] :" << sider::index::idq_count[i] << std::endl;
                }
            }
            co_yield sider::kv::opt_exit_coro_flag();
        }
        std::cout << "last put_to_batch : " << ycsb::g_statistic_data.put_count << std::endl;
        std::cout << "last get : " << ycsb::g_statistic_data.get_count << std::endl;
        std::cout << "qps      : " << (ycsb::g_statistic_data.get_count + ycsb::g_statistic_data.put_count)/ycsb::g_ycsb_options.duration << std::endl;
        g_statistic_data.stop_tick.store(spdk_get_ticks()) ;
        co_return sider::kv::opt_exit_coro_flag(sider::kv::exit_db_tag{});
    }
}
#endif //MONISM_YCSB_CORO_HH
