//

//

#ifndef SIDER_KV_DATA_COMPUTE_KEY_SEED_HH
#define SIDER_KV_DATA_COMPUTE_KEY_SEED_HH

#include "./slice.hh"

namespace sider::kv::data {
    auto
    compute_key_seed(const slice* s) {
        uint32_t res = 0;
        if ((s->len-sizeof(uint32_t)) < sizeof(uint32_t))
            memcpy(&res, s->ptr, s->len);
        else
            memcpy(&res, s->ptr, sizeof(uint32_t));
        return res;
    }
}

#endif //SIDER_KV_DATA_COMPUTE_KEY_SEED_HH
