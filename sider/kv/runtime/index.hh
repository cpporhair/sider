//

//

#ifndef SIDER_KV_DATA_INDEX_HH
#define SIDER_KV_DATA_INDEX_HH

namespace sider::kv::runtime {
    struct
    indexes {
        data::b_tree** by_core;
        std::vector<data::b_tree*> list;

        indexes()
            : by_core(new data::b_tree* [std::thread::hardware_concurrency()]{nullptr}) {
        }
    };

    indexes all_index;
}

#endif //SIDER_KV_DATA_INDEX_HH
