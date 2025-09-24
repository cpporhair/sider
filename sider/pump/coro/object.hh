//
//
//

#ifndef SIDER_OBJECT_HH
#define SIDER_OBJECT_HH

#include <memory>
#include <boost/noncopyable.hpp>
#include "base.hh"

namespace pump::coro {
    template <typename promise_t>
    struct
    co_object : boost::noncopyable , __co_flag__ {
        using promise_type  = promise_t;
        using handle_type   = typename promise_type::handle_type;

        std::unique_ptr<handle_type> unique_handle_ptr;

        co_object
            (co_object&& rhs) noexcept{
            unique_handle_ptr.reset(rhs.unique_handle_ptr.release());
        }

        explicit
        co_object
            (handle_type&& h)
            :unique_handle_ptr(new handle_type(h)){
        }

        ~co_object
            (){
            if(unique_handle_ptr)
                if(*unique_handle_ptr){
                    unique_handle_ptr->destroy();
                }
        }

        decltype(auto)
        resume(){
            unique_handle_ptr->resume();
            return promise();
        }

        auto
        address(){
            return unique_handle_ptr->address();
        }

        [[nodiscard]]
        decltype(auto)
        done()const{
            return unique_handle_ptr->done();
        }

        [[nodiscard]]
        decltype(auto)
        promise()const{
            return unique_handle_ptr->promise();
        }

        constexpr explicit
        operator bool
            () const noexcept {
            if(!unique_handle_ptr)
                return false;
            return bool(*unique_handle_ptr);
        }
    };

    template <typename T>
    struct co_value {
    };
}


#endif //SIDER_OBJECT_HH
