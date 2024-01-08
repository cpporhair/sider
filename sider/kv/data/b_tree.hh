//

//

#ifndef SIDER_KV_IMPL_INDEX_B_TREE_HH
#define SIDER_KV_IMPL_INDEX_B_TREE_HH

#include "./index.hh"

namespace sider::kv::data {
    struct
    b_tree {
        absl::btree_map<const slice*, index*, _index_compare> impl;

        void
        update(data_file* f) {
            slice *k = f->key();
            auto it = impl.find(k);
            if (it == impl.end()){
                impl.emplace(k, new index(k, f));
            }
            else {
                delete k;
                it->second->add_version(f);
            }

        }

        inline
        auto
        update(batch* b) {
            for(auto& e : b->cache)
                update(e.file);
        }

        index*
        find(slice* k) {
            auto itr = impl.find(k);
            if (itr == impl.end()) {
                std::cout << "find" << std::string(k->ptr) << " on " << this << ";" << std::endl;
                return nullptr;
            } else {
                return itr->second;
            }
        }
    };
}

#endif //SIDER_KV_IMPL_INDEX_B_TREE_HH
