//

//

#ifndef SIDER_KV_DATA_DATA_PAGE_HH
#define SIDER_KV_DATA_DATA_PAGE_HH

#include <cstring>

#include "sider/kv/runtime/const.hh"
#include "sider/kv/runtime/dma.hh"

#include "./slice.hh"

namespace sider::kv::data {

    inline
    auto
    make_data_page_payload() {
        return runtime::malloc_page();
    }

    inline
    auto
    free_data_page_payload(void* payload) {
        return runtime::free_page(payload);
    }

    struct
        data_page_payload {
        uint64_t version;
        uint32_t page_ref_count;
        uint64_t page_ref[16];
        uint32_t k_len;
        uint08_t key[1024];
        uint64_t v_len;
        uint08_t val[4096];

    };

    struct
    page_ref {
        uint64_t ssd;
        uint64_t pos;
    };

    struct
    data_page_status {
        constexpr static uint08_t wrote = uint08_t(1 << 0);
        constexpr static uint08_t error = uint08_t(1 << 1);
    };

    struct
    data_page {
        uint08_t    flg;
        const char* ssd_sn;
        uint32_t    ssd_index;
        uint64_t    pos;
        void*       payload;

        void
        add_flags(uint08_t flags) {
            flg |= flags;
        }

        void
        del_flags(uint08_t flags) {
            flg &= ~flags;
        }

        bool
        has_flags(uint08_t flags) {
            return (flg & flags) == flags;
        }

        inline
        void
        set_wrote(bool b) {
            if (b)
                add_flags(data_page_status::wrote);
            else
                del_flags(data_page_status::wrote);
        }

        inline
        bool
        is_wrote() {
            return has_flags(data_page_status::wrote);
        }

        inline
        void
        set_error() {
            add_flags(data_page_status::error);
        }

        inline
        bool
        is_error() {
            return has_flags(data_page_status::error);
        }
    };

    struct
    data_file_status {
        constexpr static uint08_t opening = uint08_t(1 << 0);
        constexpr static uint08_t caching = uint08_t(1 << 1);
    };

    // todo rename
    struct
    data_file {
        uint08_t status;
        std::atomic<uint32_t> ref_count;
        std::vector<data_page*> pages;

        void
        add_flags(uint08_t flags) {
            status |= flags;
        }

        void
        del_flags(uint08_t flags) {
            status &= ~flags;
        }

        bool
        has_flags(uint08_t flags) {
            return (status & flags) == flags;
        }

        inline
        bool
        any_error() {
            return std::all_of(pages.begin(), pages.end(),[](data_page* p){ return p->is_error(); });
        }

        auto
        add_ref() {
            ref_count.fetch_add(1);
        }

        auto
        del_ref() {
            ref_count--;
        }

        auto
        free_payload() {
            if(ref_count > 0)
                return ;
            for (auto *e : pages) {
                free_data_page_payload(e->payload);
                e->payload = nullptr;
            }
        }

        auto
        has_payload() {
            return std::all_of(pages.begin(), pages.end(), [](data_page *p) { return p->payload != nullptr; });
        }

        data_page*
        page(size_t index) const {
            return pages[index];
        }

        void*
        positioning(uint64_t pos) const {
            return (char *) (page(pos / runtime::data_page_size)->payload) + (pos % runtime::data_page_size);
        }

        auto
        write_one_page(uint64_t pos, const void* data, uint32_t data_len) {
            uint32_t len = std::min(data_len, runtime::data_page_size - (uint32_t)(pos % runtime::data_page_size));
            memcpy(positioning(pos), data, len);
            return len;
        }

        auto
        write_memory(uint64_t pos, const void* data, uint64_t data_len) {
            uint64_t wrote = 0;
            while (wrote < data_len)
                wrote += write_one_page(pos, (char *) (data) + wrote, data_len - wrote);
            return wrote;
        }

        auto
        write_data(uint64_t pos, const auto& data) requires(!std::is_pointer_v<__typ__(data)>) {
            return write_memory(pos,(void*) &data, sizeof(__typ__(data)));
        }

        auto
        read_one_page(uint64_t pos, char *buf, uint64_t buf_len) const {
            uint64_t len = std::min(buf_len, runtime::data_page_size - (pos % runtime::data_page_size));
            memcpy(buf, positioning(pos), len);
            return len;
        }

        auto
        read_memory(uint64_t pos, char *buf, uint64_t len) const {
            uint64_t count = 0;
            while (count < len)
                count += read_one_page(pos, buf + count, len - count);
            return count;
        }

        auto
        read_data(uint64_t pos, auto& obj) const {
            return read_memory(pos, (char*)&obj, sizeof(__typ__(obj)));
        }

        inline
        uint64_t
        key_pos() const {
            return sizeof(uint64_t) + sizeof(uint32_t) + pages.size() * sizeof(page_ref);
        }

        inline
        uint64_t
        val_pos() const {
            uint64_t pos = key_pos();
            uint64_t len = 0;
            read_data(pos, len);
            return pos + len;
        }

        inline
        auto
        write_version(uint64_t ver) {
            write_data(0, ver);
        }

        inline
        auto
        write_key(slice *k){
            write_memory(key_pos(), k, k->len);
        }

        inline
        auto
        write_val(slice *v){
            write_memory(val_pos(), v, v->len);
        }

        inline
        auto
        write_tombstone() {
            uint64_t u = 0;
            write_val((slice*)(&u));
        }

        inline
        uint64_t
        version() {
            return *((uint64_t*)(page(0)->payload));
        }

        inline
        uint64_t
        key_len() const {
            uint64_t len = 0;
            read_data(key_pos(), len);
            return len;
        }

        inline
        slice*
        key() const {
            uint64_t len = key_len();
            slice* res = (slice*)malloc(len);
            read_memory(key_pos(), (char*)res, len);
            return res;
        }

        inline
        uint64_t
        val_len() const {
            uint64_t len = 0;
            read_data(val_pos(), len);
            return len;
        }

        inline
        slice*
        val() const {
            uint64_t len = val_len();
            slice* res = (slice*)malloc(len);
            read_memory(val_pos(), (char*)res, len);
            return res;
        }

        inline
        uint32_t
        key_seed() const {
            uint64_t len = key_len();
            uint32_t res = 0;
            read_memory(key_pos() + sizeof(uint64_t), (char *) &res, (len - sizeof(uint64_t)) < sizeof(uint32_t) ? (len - sizeof(uint64_t)) : sizeof(uint32_t));
            return res;
        }
    };

    struct
    __ncp__(data_file_ref) {
        data_file* file;

        data_file_ref(data_file *f)
            : file(f) {
            file->ref_count++;
        }

        data_file_ref(data_file_ref &&rhs)
            : file(__fwd__(rhs.file)) {
            file->ref_count++;
        }

        ~data_file_ref() {
            file->ref_count--;
            file->free_payload();
        }
    };

    auto
    make_data_file(uint32_t page_count) {
        auto* file = new data_file;
        for (uint32_t i = 0; i < page_count; ++i)
            file->pages.push_back(
                new data_page{
                    0,
                    0,
                    0,
                    0,
                    make_data_page_payload()
                }
            );
        assert(file!= nullptr);
        return file;
    }

    auto
    make_data_file(uint32_t k_len, uint64_t v_len) {

        uint64_t l1 = k_len + v_len + sizeof(uint32_t) + sizeof(uint64_t);
        uint64_t l2 = runtime::data_page_size - sizeof(page_ref);
        uint64_t l3 = l1 % l2 == 0 ? l1 / l2 : l1 / l2 + 1;

        return make_data_file(l3);
    }
}

#endif //SIDER_KV_DATA_DATA_PAGE_HH
