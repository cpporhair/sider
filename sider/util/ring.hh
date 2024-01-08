//
//
//

#ifndef ZKV_RING_HH
#define ZKV_RING_HH

#include <memory>
#include <optional>

namespace sider::util {
    template <typename unit,size_t unit_size= sizeof(unit)>
    requires std::is_pointer_v<unit>
    struct
    ring_queue {
        uint8_t*    data{};
        uint8_t*    head{};
        uint8_t*    tail{};
        uint64_t    size{};

        inline
        constexpr static
        size_t
        get_unit_size(){
            return unit_size;
        }

        inline
        size_t
        capacity()const{
            return size - unit_size;
        }

        inline
        size_t
        free_bytes()const{
            if (this->head >= this->tail)
                return capacity() - (this->head - this->tail);
            else
                return this->tail - this->head - unit_size;
        }

        inline
        size_t
        used_bytes()const{
            return capacity() - free_bytes();
        }

        inline
        size_t
        count()const {
            return used_bytes() / get_unit_size();
        }

        inline
        bool
        full() {
            return free_bytes() == 0;
        }

        inline
        bool
        empty() const {
            return free_bytes() == capacity();
        }

        inline
        const uint8_t *
        end() const {
            return this->data + size;
        }

        inline
        uint8_t*
        advance_head(){
            this->head += unit_size;
            if (this->head == end())
                this->head = this->data;
            return this->head;
        }
        inline
        uint8_t*
        advance_tail(){
            this->tail += unit_size;
            if (this->tail == end())[[unlikely]]
                this->tail = this->data;
            return this->tail;
        }

    };

    template <typename T>
    void
    free_ring(ring_queue<T>* rb) {
        delete rb->data;
        rb->head = rb->tail = rb->data = nullptr;
        rb->size = 0;
        delete rb;
    }

    template <typename T>
    void
    reset_ring(ring_queue<T>* rb){
        rb->head = rb->tail = rb->data;
    }

    template <typename T>
    ring_queue<T>*
    make_ring(size_t count){
        auto rb = new ring_queue<T>();
        count = count * rb->get_unit_size();

        if (rb) {
            rb->size = count + rb->get_unit_size();
            rb->data = new uint8_t[count];

            if (rb->data)
                reset_ring(rb);
            else {
                delete (rb);
                return nullptr;
            }
        }
        return rb;
    }

    template <typename T>
    bool
    push_ring(ring_queue<T>* dst, const void *src) {

        if(dst->full())
            return false;

        memcpy(dst->head, &src, dst->get_unit_size());

        dst->advance_head();

        return true;
    }

    template <typename T>
    inline
    T
    poll_ring(ring_queue<T>* src) {
        auto res = *((T*)(src->tail));
        src->advance_tail();
        return res;
    }

    template<typename T>
    inline
    void
    poll_ring(ring_queue<T> *src, T &t) {
        t = *((T *) (src->tail));
        src->advance_tail();
    }

    template <typename T>
    std::optional<T>
    maybe_poll_ring(ring_queue<T>* src) {
        if (src->empty())
            return std::nullopt;
        return std::optional<T>(poll_ring(src));
    }

    template<typename T>
    struct
    poll_iterator {
        using value_type = T;
        using difference_type   = std::ptrdiff_t;

        ring_queue<value_type>* ring{};
        std::optional<value_type> t;

        poll_iterator
        (poll_iterator& o) = delete;

        poll_iterator
            ()
            :ring(nullptr)
            ,t(std::nullopt){}

        explicit
        poll_iterator
            (ring_queue<value_type>* _ring)
            :ring(_ring)
            ,t(poll_ring(ring)){
        }

        poll_iterator(poll_iterator&& rhs) noexcept
            :ring(rhs.ring)
            ,t(rhs.t){
            rhs.t = std::nullopt;
            rhs.ring= nullptr;
        }

        poll_iterator&
        operator
        ++ () {
            t = poll_ring(ring);
            return *this;
        }

        value_type
        operator
        * () const noexcept {
            return t.value();
        }


        poll_iterator&
        operator
        = (poll_iterator&& o) noexcept {
            if(this!=std::addressof(o)){
                this->~poll_iterator();
                new(this) poll_iterator(__fwd__(o));
            }
            return *this;
        }

        void
        operator
        ++ (int) {
            (void)this->operator++();
        }

        friend bool
        operator
        == (poll_iterator const& it, auto end) noexcept {
            return !it.t.has_value();
        }
    };

    template<typename T>
    struct
    ring_poller {
        using value_t = T;
        ring_queue<value_t>* ring;

        ring_poller():ring(nullptr){

        }


        ring_poller
        (ring_poller& o) = delete;

        explicit
        ring_poller(ring_queue<value_t>* _r)
        :ring(_r){
        }

        ring_poller(ring_poller&& rhs)noexcept{
            this->ring = rhs.ring;
            rhs.ring= nullptr;
        }

        poll_iterator<T>
        begin(){
            return poll_iterator<std::decay_t<T>>(ring);
        }
        [[nodiscard]]
        std::default_sentinel_t
        end() const noexcept {
            return std::default_sentinel_t() ;
        }

        std::add_lvalue_reference_t<ring_poller>
        operator
        = (ring_poller const&) = delete;

        ring_poller&
        operator
        = (ring_poller&& o) noexcept {
            this->ring = o.ring;
            o.ring= nullptr;
            return *this;
        }
    };

    template <typename T>
    struct
    const_ring_queue {
        ring_queue<T>* inner;
        const_ring_queue
            (ring_queue<T>* r)
            :inner(r){
        }
    };

    template <typename T>
    auto
    get_first_and_move_to_tail(const_ring_queue<T>* q){
        auto* z = poll_ring(q->inner);
        push_ring(q->inner,z);
        return z;
    }
}

template<class T>
inline constexpr bool std::ranges::enable_view<sider::util::ring_poller<T>> = true;


#endif //ZKV_RING_HH
