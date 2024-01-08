//

//

#ifndef SIDER_KV_INDEX_SCAN_HH
#define SIDER_KV_INDEX_SCAN_HH

#include "./root.hh"

namespace sider::kv::index {
    auto
    scan(data::slice* key, uint64_t read_sn, uint64_t free_sn, uint32_t max) {
        /*
        return pump::just()
            >> pump::for_each(runtime::index_schedulers.list)
            >> pump::concurrent()
            >> pump::then([](index::scheduler *scheduler) {
                return scheduler->get_b_tree();
            })
            >> pump::flat()
            >> pump::get_context<data::batch *>()
            >> pump::then([key, read_sn, free_sn, max](data::batch *batch, data::b_tree *btree) {
                auto it = btree->impl.lower_bound(key);
                uint32_t count = 0;
                while (it!= btree->impl.end() && count < max) {
                    auto* v = it->second->get_version(read_sn);
                    if(!v)
                        continue;
                    batch->current_scan_env.push_result(data::key_value{v->file});
                    count++;
                }
            })
            >> pump::reduce()
            >> pump::get_context<data::batch *>()
            >> pump::then([max](data::batch *batch, auto&& ...){
                batch->current_scan_env.collect(max);
            });
        */
    }
}

#endif //SIDER_KV_INDEX_SCAN_HH
