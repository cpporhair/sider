//

//

#ifndef SIDER_KV_DATA_SNAPSHOT_HH
#define SIDER_KV_DATA_SNAPSHOT_HH

namespace sider::kv::data {
    struct
    snapshot {
    private:
        uint64_t serial_number;
        std::atomic<uint64_t> ref;
        //uint32_t ref_count;
    public:
        snapshot(uint64_t seq)
            : serial_number(seq) {
        }
        ~snapshot(){
        }

        inline
        auto
        referencing() {
            ref++;;
        }

        inline
        auto
        release() {
            return ref.fetch_sub(1) == 1;
        }
        inline
        bool
        free() {
            return ref == 0;
        }

        constexpr
        inline
        auto
        get_serial_number() const {
            return serial_number;
        }
    };

}

#endif //SIDER_KV_DATA_SNAPSHOT_HH
