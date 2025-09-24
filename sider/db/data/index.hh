#ifndef SIDER_DB_DATA_INDEX_HH
#define SIDER_DB_DATA_INDEX_HH

#include <absl/container/btree_map.h>

#include "./kv.hh"

namespace sider::db::data {
    struct
    key_compare : public std::less<slice*>{
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

    struct
    index_memo {
        std::variant<slice *, uint64_t> val;
    };

    struct
    index {
        absl::btree_map<const slice*, index_memo*, key_compare> impl;
    };
}

#endif //SIDER_DB_DATA_INDEX_HH
