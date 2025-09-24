//
//
//

#ifndef SIDER_GENERATOR_HH
#define SIDER_GENERATOR_HH

#include <memory>
#include <boost/noncopyable.hpp>
#include "promise.hh"

namespace pump::coro {
    template <typename co_object_t>
    struct iterator : boost::noncopyable {
        co_object_t handle;
        using value_type        = std::decay_t<decltype(handle.promise().take())>;
        using reference_type = std::add_lvalue_reference_t<value_type>;
        using pointer_type = std::add_pointer_t<value_type>;
        using difference_type   = std::ptrdiff_t;

        iterator
            ()
            :handle(nullptr){
        }

        iterator
            (iterator& o) noexcept
            : handle(__fwd__(o.handle)){
        }

        iterator(iterator const&) = delete;

        iterator
            (iterator&& o) noexcept
            : handle(__fwd__(o.handle)){
        }

        explicit
        iterator
            (co_object_t&& h)
            :handle(__fwd__(h)){
        }

        iterator&
        operator=(iterator const&) = delete;

        iterator&
        operator
        = (iterator&& o) noexcept {
            if(this!=std::addressof(o)){
                this->~iterator();
                new(this) iterator(__fwd__(o));
            }
            return *this;
        }

        iterator&
        operator
        ++ () {
            handle.resume();
            if (handle.done()) {
                handle.promise().rethrow_if_exception();
            }
            return *this;
        }

        void
        operator
        ++ (int) {
            (void)this->operator++();
        }

        reference_type
        operator
        * () const noexcept {
            return handle.promise().ref();
        }

        std::add_lvalue_reference_t<value_type>
        operator
        ()() {
            return ++(*this);
        }


        friend bool
        operator
        == (iterator const& it, std::default_sentinel_t end) noexcept {
            return (!it.handle || it.handle.done());
        }

    };

    template <typename co_object_t>
    struct co_view_able : boost::noncopyable {
        co_object_t handle;
        using value_type  = std::decay_t<decltype(handle.promise().take())>;

        co_view_able
            ()
            :handle(nullptr){
        }

        co_view_able
            (co_view_able& o) = delete;
        co_view_able
            (co_view_able const&) = delete;

        co_view_able
            (co_view_able&& o) noexcept
            : handle(__fwd__(o.handle)) {
        }

        explicit
        co_view_able
            (co_object_t&& h)
            :handle(__fwd__(h)){
        }

        virtual
        ~co_view_able
            ()= default;

        co_view_able&
        operator
        = (co_view_able const&) = delete;

        co_view_able&
        operator
        = (co_view_able&& o) noexcept {
            std::swap(handle,o.handle);
            return *this;
        }

        iterator<co_object_t>
        begin() {
            handle.resume();
            if (handle.done()) {
                handle.promise().rethrow_if_exception();
            }
            return iterator(__mov__(handle));
        }

        [[nodiscard]]
        std::default_sentinel_t
        end() const noexcept {
            return {} ;
        }

        void swap(co_view_able& other) noexcept {
            std::swap(handle, other.handle_);
        }
    };
}

template<typename T>
inline constexpr bool std::ranges::enable_view<pump::coro::co_view_able<T>> = true;

#endif //SIDER_GENERATOR_HH
