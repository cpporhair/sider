#ifndef SIDER_DB_DATA_KV_HH
#define SIDER_DB_DATA_KV_HH

#include <memory>
#include <variant>
#include "./slice.hh"

namespace sider::db::data {
    struct
    key_value {
        slice *key;
        slice *val;
    };
}

#endif //SIDER_DB_DATA_KV_HH
