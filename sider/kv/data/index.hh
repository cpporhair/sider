//

//

#ifndef SIDER_KV_IMPL_INDEX_INDEX_HH
#define SIDER_KV_IMPL_INDEX_INDEX_HH

#include <set>

#include <absl/container/btree_map.h>
#include "./kv.hh"

namespace sider::kv::data {

    struct
    version {
        std::list<void*> page_waiters;
        const uint64_t sn;
        data_file* file;

        version(data_file* f)
            : file(f)
            , sn(f->version()){
        }

        version(uint64_t _sn)
            : file(new data_file)
            , sn(_sn){
        }

        inline
        uint64_t
        get_sn() const {
            return sn;
        }
    };

    struct
    version_compare {
        bool
        operator()(const version *lhs, const version *rhs) const {
            return rhs->get_sn() < lhs->get_sn();
        }
    };

    struct
    index {
        const slice* key;
        std::set<version*, version_compare> versions;

        explicit
        index(data_file *file)
            : key(file->key())
            , versions{new version{file}} {
        }

        index(slice* k, data_file *file)
            : key(k)
            , versions{new version{file}} {
        }

        index(slice* k, version* v)
            : key(k)
            , versions{v} {
        }

        void
        add_version(data_file* f) {
            auto* v = new version(f);
            versions.emplace(v);
        }

        version*
        get_version(uint64_t sn) {
            for(version* e : versions)
                if (e->get_sn() <= sn)
                    return e;
            return nullptr;
        }
    };

    struct
    _index_compare : public std::less<slice*>{
        constexpr
        bool
        operator()(const slice *lhs, const slice *rhs) const noexcept {
            if (lhs == rhs)
                return false;
            if (lhs == nullptr)
                return true;
            if (rhs == nullptr)
                return false;
            uint32_t n = std::min(lhs->ptr_size(), rhs->ptr_size());
            for (uint32_t i = 0; i < n; ++i) {
                if (lhs->ptr[i] != rhs->ptr[i])
                    return lhs->ptr[i] < rhs->ptr[i];
            }
            return lhs->len < rhs->len;
        }
    };

    constexpr inline _index_compare index_compare{};



}

#endif //SIDER_KV_IMPL_INDEX_INDEX_HH
