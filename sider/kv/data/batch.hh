//

//

#ifndef SIDER_KV_DATA_BATCH_HH
#define SIDER_KV_DATA_BATCH_HH

#include <list>
#include <thread>

#include "sider/util/macro.hh"

#include "sider/kv/runtime/const.hh"

#include "./kv.hh"
#include "./snapshot.hh"
#include "./index.hh"

namespace sider::kv::data {

    enum struct
    batch_status {
        unpublished,
        published
    };

    struct
    scan_result {
        const slice* k;
        key_value kv;

        bool
        operator < (const scan_result& rhs)const {
            return index_compare(k, rhs.k);
        }
    };


    struct
    scan_env_per_core {
        const data::slice* last_key;
        std::list<scan_result> result;
    };

    struct
    scan_env {
        uint32_t max_count;
        data::slice* start_key;
        std::set<scan_result> collected_scan_res;
        std::vector<scan_env_per_core> scan_res_by_core;

        scan_env()
            : scan_res_by_core(std::thread::hardware_concurrency())
            , max_count(50)
            , start_key(nullptr){
        }

        void
        reset(const char* k) {
            if(start_key)
                delete start_key;
            start_key = make_slice(k);
            for (uint32_t i = 0; i < scan_res_by_core.size(); ++i)
                scan_res_by_core[i].result.clear();
        }

        auto
        push_result(const slice* k, key_value&& kv){
            scan_res_by_core[spdk_env_get_current_core()].result
                .emplace_back(k, key_value{kv.file});
            scan_res_by_core[spdk_env_get_current_core()].last_key
                = scan_res_by_core[spdk_env_get_current_core()].result.back().k;
        }

        auto
        collect(uint32_t max) {
            while(collected_scan_res.size() < max) {
                const data::slice* min = nullptr;
                uint32_t pos = UINT32_MAX;
                for (uint32_t i = 0; i < scan_res_by_core.size(); ++i) {
                    if(scan_res_by_core[i].result.empty())
                        continue;
                    const data::slice *key = scan_res_by_core[i].result.front().k;
                    if (!min) {
                        min = key;
                        pos = i;
                    }
                    else if (index_compare(key, min)) {
                        pos = i;
                        min = key;
                    }
                }
                collected_scan_res.emplace(
                    scan_res_by_core[pos].result.front().k,
                    key_value(scan_res_by_core[pos].result.front().kv.file)
                );
                scan_res_by_core[pos].result.pop_front();
            }
        }
    };

    std::atomic<uint64_t> batch_count = 0;
    struct
    __ncp__(batch) {
        bool applied;
        bool published;
        snapshot* put_snapshot;
        snapshot* get_snapshot;
        uint64_t last_free_sn;
        uint64_t page_count;
        scan_env current_scan_env;
        std::list<key_value> cache;

        batch()
            : published(false)
            , applied(false)
            , page_count(0)
            , put_snapshot(nullptr)
            , get_snapshot(nullptr)
            , last_free_sn(0)
            , cache()
            , current_scan_env(){
            batch_count++;
        }

        ~batch() {
            batch_count--;
            cache.clear();
        }

        void
        put(key_value&& kv) {
            page_count += kv.file->pages.size();
            cache.emplace_back(__fwd__(kv));
        }

        void
        update_put_sn() {
            for(auto& kv : cache) {
                kv.file->write_version(put_snapshot->get_serial_number());
            }
        }

        inline
        bool
        any_error() {
            return std::any_of(cache.begin(), cache.end(), [](key_value &kv) { return kv.file->any_error(); });
        }

        inline
        void
        release_payload() {
            std::for_each(cache.begin(), cache.end(), [](key_value &kv) { kv.file->free_payload(); });
        }
    };
}

#endif //SIDER_KV_DATA_BATCH_HH
