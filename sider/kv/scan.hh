//

//

#ifndef SIDER_KV_SCAN_HH
#define SIDER_KV_SCAN_HH

#include "./index/scan.hh"

namespace sider::kv{

    auto
    scan() {
        return pump::for_each(runtime::index_schedulers.list)
            >> pump::concurrent()
            >> pump::then([](index::scheduler *scheduler) {
                return scheduler->get_b_tree();
            })
            >> pump::flat()
            >> pump::get_context<data::batch *>()
            >> pump::then([](data::batch *batch, data::b_tree *btree) {
                auto it = btree->impl.lower_bound(batch->current_scan_env.start_key);
                uint32_t count = 0;
                while (it!= btree->impl.end() && count < batch->current_scan_env.max_count) {
                    auto* i = it->second;
                    auto* v = i->get_version(batch->get_snapshot->get_serial_number());
                    it++;
                    if(!v)
                        continue;
                    batch->current_scan_env.push_result(i->key, data::key_value{v->file});
                    count++;
                }
            })
            >> pump::reduce()
            >> pump::get_context<data::batch *>()
            >> pump::then([](data::batch *batch, ...){
                batch->current_scan_env.collect(batch->current_scan_env.max_count);
            });
    }

    auto
    start_scan(const char* k) {
        return pump::get_context<data::batch*>()
            >> pump::then([k](data::batch* b, ...){
                b->current_scan_env.reset(k);
            })
            >> scan();
    }

    auto
    start_scan() {
        return pump::get_context<data::batch*>()
            >> pump::then([](data::batch* b, std::string&& s){
                b->current_scan_env.reset(s.c_str());
            })
            >> scan();
    }

    auto
    next() {
        return pump::get_context<data::batch*>()
            >> pump::then([](data::batch* b, ...){
                return b->current_scan_env.collected_scan_res.empty();
            })
            >> pump::visit()
            >> pump::then([](auto&& arg){
                if constexpr (std::is_same_v<std::true_type, __typ__(arg)>)
                    return pump::just() >> scan();
                else
                    return pump::just();
            })
            >> pump::flat()
            >> pump::get_context<data::batch*>()
            >> pump::then([](data::batch* b){
                if (!b->current_scan_env.collected_scan_res.empty()) {
                    auto it = b->current_scan_env.collected_scan_res.begin();
                    auto k = it->k;
                    auto f = it->kv.file;
                    b->current_scan_env.collected_scan_res.erase(it);
                    return data::scan_result{k, data::key_value{f}};
                }
                else {
                    return data::scan_result{nullptr, {nullptr}};
                }
            });
    }
}

#endif //SIDER_KV_SCAN_HH
