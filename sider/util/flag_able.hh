//
//
//

#ifndef SIDER_UTIL_FLAG_ABLE_HH
#define SIDER_UTIL_FLAG_ABLE_HH

namespace sider::util {
    template <typename flag_t>
    struct
    flag_able {
        flag_t flg;
        inline
        void
        add_flags(flag_t flags) {
            flg |= flags;
        }

        inline
        void
        del_flags(flag_t flags) {
            flg &= ~flags;
        }

        inline
        bool
        has_flags(flag_t flags) const {
            return (flg & flags) == flags;
        }
    };
}

#endif //SIDER_UTIL_FLAG_ABLE_HH
