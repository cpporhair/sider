//
//
//

#ifndef SIDER_UTIL_SLICE_HH
#define SIDER_UTIL_SLICE_HH

#include <cstring>

namespace sider::util {

    struct
    slice {
        uint64_t    len;
        char        ptr[];

        uint64_t
        ptr_size() const {
            return len - sizeof(uint64_t);
        }

        inline friend
        auto
        operator == (const slice& s, const char* k) {
            if (s.ptr_size() != std::strlen(k))
                return false;
            for (uint64_t i = 0; i < s.ptr_size(); ++i) {
                if (s.ptr[i] != k[i])
                    return false;
            }

            return true;
        }

        inline
        auto
        equal(const char* k) const {
            return (*this) == k;
        }

        static
        slice *
        build_from(const char *s, uint64_t l) {
            slice *res = (slice *) new char[l + sizeof(uint64_t)];
            res->len = l + sizeof(uint64_t);
            memcpy(&res->ptr, s, l);
            return res;
        }

        inline
        static
        slice*
        from_string(const char* s) {
            return build_from(s, strlen(s));
        }

        inline
        static
        slice*
        from_string(const std::string&& s) {
            return build_from(s.c_str(), s.length());
        }
    };
}

#endif //SIDER_UTIL_SLICE_HH
