//
//
//

#ifndef SIDER_UTILS_UNIQUE_BOX_HH
#define SIDER_UTILS_UNIQUE_BOX_HH

#include <memory>
#include <boost/noncopyable.hpp>

namespace sider::utils {
    template <typename T>
    struct
    unique_box : boost::noncopyable {
        std::unique_ptr<T> detail;

        ~unique_box() {
            if (detail)
                delete detail.release();
        }
        explicit
        unique_box(auto&& ...args)
            : detail(new T(__fwd__(args)...)){
        }

        unique_box(unique_box&& o) noexcept
            : detail(o.detail.release()){
        }

        explicit
        unique_box(T* t)
            : detail(t){
        }

        auto&
        operator = (unique_box&& o)  noexcept {
            detail.reset(o.detail.release());
            return *this;
        }
    };
}
#endif //SIDER_UTILS_UNIQUE_BOX_HH
