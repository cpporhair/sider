//

//

#ifndef SIDER_KV_PUT_HH
#define SIDER_KV_PUT_HH

#include "sider/pump/get_context.hh"
#include "sider/pump/then.hh"

#include "./data/batch.hh"
#include "./data/kv.hh"

namespace sider::kv {

    auto
    put() {
        return pump::get_context<data::batch*>()
            >> pump::then([](data::batch* b, data::key_value&& kv) mutable {
                b->put(__fwd__(kv));
            });
    }

    inline
    auto
    put(data::key_value&& kv) {
        return pump::forward_value(__fwd__(kv)) >> put();
    }

    auto
    del() {
        return pump::get_context<data::batch*>()
            >> pump::then([](data::batch* b, const char* key) mutable {
                b->put(data::make_tombstone(data::make_slice(key)));
            });
    }
}

#endif //SIDER_KV_PUT_HH
