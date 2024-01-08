//

//

#ifndef SIDER_KV_MAKE_KV_HH
#define SIDER_KV_MAKE_KV_HH

#include "./data/kv.hh"

namespace sider::kv {
    inline
    auto
    make_kv(const char* k, const char* v) {
        return data::make_kv(data::make_slice(k), data::make_slice(v));
    }

    inline
    auto
    make_slice(const char* k) {
        return data::make_slice(k);
    }
}

#endif //SIDER_KV_MAKE_KV_HH
