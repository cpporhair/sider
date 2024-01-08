//

//

#ifndef SIDER_KV_BATCH_SNAPSHOT_MANAGER_HH
#define SIDER_KV_BATCH_SNAPSHOT_MANAGER_HH


#include "./publish_req.hh"
#include "./request_task.hh"

namespace sider::kv::batch {
    struct
    snapshot_manager {
        uint64_t current_serial_number;
        uint64_t last_free_serial_number;
        data::snapshot* current_readable;
        std::list<data::snapshot*> old_snapshots;
        publish_task_set un_publish_seqs;

        snapshot_manager()
            : current_serial_number(1)
            , current_readable(new data::snapshot(0)) {
        }

        auto
        gc() {
            if(old_snapshots.empty())[[unlikely]]
                return ;
            auto it = old_snapshots.begin();
            while (it != old_snapshots.end()) {
                if ((*it)->free()) {
                    last_free_serial_number = (*it)->get_serial_number();
                    it = old_snapshots.erase(it);
                }
                else {
                    it++;
                }
            }
        }

        auto
        allocate_put_id(request_put_id_task* task) {
            auto* res = new data::snapshot(current_serial_number++);
            res->referencing();
            task->cb(res, last_free_serial_number);
        }

        auto
        request_read(request_get_id_task* task) {
            current_readable->referencing();
            task->cb(current_readable, last_free_serial_number);
        }

        auto
        set_new_snapshot(data::snapshot* s) {
            auto *old = current_readable;
            current_readable = s;
            if (!old)
                return ;
            if (!old->free()) {
                old_snapshots.push_back(old);
            }
            else {
                if(old_snapshots.empty())
                    last_free_serial_number = old->get_serial_number();
                delete old;
            }
        }

        auto
        try_publish() {
            while (auto *ups = un_publish_seqs.try_pop(current_readable->get_serial_number() + 1)) {
                set_new_snapshot(ups->batch->put_snapshot);
                ups->batch->put_snapshot = nullptr;
                ups->batch->published = true;
                ups->cb();
                delete ups;
            }
        }

        inline
        auto
        push_in_unpublished_queue(publish_req* ups) {
            ups->batch->get_snapshot->release();
            ups->batch->put_snapshot->release();
            un_publish_seqs.add(ups);
        }
    };
}

#endif //SIDER_KV_BATCH_SNAPSHOT_MANAGER_HH
