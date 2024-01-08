//

//

#ifndef SIDER_KV_BATCH_PUBLISH_TASK_HH
#define SIDER_KV_BATCH_PUBLISH_TASK_HH


#include <set>

#include "sider/kv/data/batch.hh"

namespace sider::kv::batch {
    struct
    publish_req {
        data::batch* batch;
        std::move_only_function<void()> cb{};

        explicit
        publish_req()
            : batch(nullptr)
            , cb(){
        }

        template<typename func_t>
        explicit
        publish_req(data::batch* b, func_t&& func)
            : batch(b)
            , cb(__fwd__(func)){
        }
    };

    struct
    compare_publish_req_by_put {
        constexpr
        bool
        operator () (const publish_req* lhs, const publish_req* rhs) const noexcept {
            return lhs->batch->put_snapshot->get_serial_number() < rhs->batch->put_snapshot->get_serial_number();
        }
    };

    struct
    publish_task_set {
        std::set<publish_req*, compare_publish_req_by_put> sets;

        publish_task_set(){
        }
        publish_req*
        try_pop(uint64_t id) {
            if(sets.empty())[[unlikely]]
                return nullptr;
            runtime::g_task_info.wait_batch = id;
            if ((*sets.begin())->batch->put_snapshot->get_serial_number() == id) {
                auto* res = (*sets.begin());
                sets.erase(sets.begin());
                return res;
            }
            return nullptr;
        }

        auto
        add(publish_req* ups) {
            sets.emplace(ups);
        }
    };
}

#endif //SIDER_KV_BATCH_PUBLISH_TASK_HH
