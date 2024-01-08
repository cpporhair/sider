//

//

#ifndef SIDER_KV_DATA_SLOTS_HH
#define SIDER_KV_DATA_SLOTS_HH

#include <set>

#include "./batch.hh"
#include "./write_span.hh"

namespace sider::kv::data {
    struct
    slot {
        uint64_t page_pos{};
        uint64_t page_len{};

        ~slot(){
        }

        slot(uint64_t p, uint64_t l)
            : page_pos(p)
            , page_len(l) {
        }

        inline
        bool
        empty() {
            return page_len == 0;
        }

        decltype(auto)
        allocate(uint32_t len_by_page) {
            auto old = page_pos;
            this->page_pos += len_by_page;
            this->page_len -= len_by_page;
            return slot{old, len_by_page};
        }

        inline
        bool
        digging_able(uint64_t pos) {
            return (page_pos <= pos) && (pos < page_pos + page_len);
        }

        auto
        digging(uint64_t pos) {
            if (pos == page_pos) {
                page_pos++;
                page_len--;
                return slot{0,0};
            }

            if (pos == page_pos + page_len - 1) {
                page_len--;
                return slot{0,0};
            }

            uint64_t old = page_pos;
            page_pos = pos + 1;
            page_len = page_len - (pos + 1 - old);

            return slot{old, (pos - old)};
        }

        auto
        decrease(uint32_t len_by_page) {
            this->page_pos += len_by_page;
            this->page_len -= len_by_page;
        }

        void
        merge(const slot& slot) {
            page_pos = std::min(page_pos, slot.page_pos);
            page_len = page_len + slot.page_len;
        }

        [[nodiscard]]
        constexpr
        bool
        merge_able(const slot& slot) const {
            return ((page_pos + page_len) == slot.page_pos) || ((slot.page_pos + slot.page_len) == page_pos);
        }

        constexpr
        bool
        try_merge(slot& slot) {
            if(merge_able(slot))
                return false;
            merge(slot);
        }
    };

    struct
    compare_slot_by_pos {
        constexpr
        bool
        operator()(const slot &lhs, const slot &rhs) const {
            return lhs.page_pos < rhs.page_pos;
        }
    };

    struct
    compare_slot_by_len {
        constexpr
        bool
        operator()(const slot &lhs, const slot &rhs) const {
            return lhs.page_len < rhs.page_len;
        }
    };

    struct
    slot_manager {
        const char* ssd_sn;
        uint32_t ssd_index;
        uint64_t capacity;
        std::set<slot, compare_slot_by_pos> free_slots;
        slot current_slot;
        std::set<slot, compare_slot_by_pos> temp_slots;

        slot_manager(const char *sn, uint32_t i, uint64_t size = UINT64_MAX)
            : current_slot{0, size}
            , capacity(size)
            , ssd_sn(sn)
            , ssd_index(i)
            , temp_slots() {

        }

        void
        change_current_slot(){
            current_slot.page_pos = 0;
            current_slot.page_len = 0;
            if (free_slots.empty())
                return;
            current_slot = *free_slots.begin();
            free_slots.erase(free_slots.begin());

            while (free_slots.empty() && current_slot.merge_able(*free_slots.begin())) {
                current_slot.merge(*free_slots.begin());
                free_slots.erase(free_slots.begin());
            }
        }

        auto
        init() {
            if (temp_slots.empty())
                return ;
            uint64_t used_page_count = 0;
            for(auto& e : temp_slots) {
                used_page_count += e.page_len;
            }

            std::cout << "used page len0 = " << used_page_count << std::endl;

            auto it = temp_slots.begin();
            while (it != temp_slots.end()) {
                slot slot0(it->page_pos, it->page_len);
                it++;
                if (it == temp_slots.end()) {
                    current_slot.page_pos = slot0.page_pos + slot0.page_len;
                    current_slot.page_len = capacity - current_slot.page_pos;
                    break;
                }
                else {
                    slot slot1{slot0.page_pos + slot0.page_len, it->page_pos - (slot0.page_pos + slot0.page_len)};
                    free_slots.emplace(slot1);
                }
            }
            temp_slots.clear();
            uint64_t free_page_count = 0;
            for(auto& e : free_slots) {
                free_page_count += e.page_len;
            }
            free_page_count += current_slot.page_len;
            std::cout << "used page len = " << capacity - free_page_count << std::endl;
        }

        void
        set_slot_allocated(data::slot& this_slot) {
            if (temp_slots.empty()) {
                temp_slots.insert(data::slot{this_slot.page_pos, this_slot.page_len});
                return;
            }

            auto it0 = temp_slots.lower_bound(this_slot);
            if (it0 == temp_slots.end()) {
                it0--;
                if (it0 == temp_slots.end()) {
                    temp_slots.insert(data::slot{this_slot.page_pos, this_slot.page_len});
                }
                else if (it0->merge_able(this_slot)) {
                    this_slot.merge(*it0);
                    temp_slots.erase(it0);
                    set_slot_allocated(this_slot);
                }
                else {
                    temp_slots.insert(data::slot{this_slot.page_pos, this_slot.page_len});
                }
            }
            else if (it0->merge_able(this_slot)) {
                this_slot.merge(*it0);
                temp_slots.erase(it0);
                set_slot_allocated(this_slot);
            }
            else {
                it0--;
                if (it0 == temp_slots.end()) {
                    temp_slots.insert(data::slot{this_slot.page_pos, this_slot.page_len});
                }
                else if (it0->merge_able(this_slot)) {
                    this_slot.merge(*it0);
                    temp_slots.erase(it0);
                    set_slot_allocated(this_slot);
                }
                else {
                    temp_slots.insert(data::slot{this_slot.page_pos, this_slot.page_len});
                }
            }
        }

        auto
        set_page_allocated(uint64_t pos) {
            data::slot this_slot{pos, 1};
            set_slot_allocated(this_slot);
        }

        auto
        allocate_one_page() {
            if (current_slot.empty())
                change_current_slot();
            capacity--;
            return current_slot.allocate(1);
        }

        inline
        bool
        allocate_able(uint64_t page_count){
            return page_count <= capacity;
        }

        bool
        allocate(data::write_span_list& w, data::batch* b) {
            if (!allocate_able(b->page_count))
                return false;
            for(auto& e : b->cache) {
                for (auto *p : e.file->pages) {
                    auto s = allocate_one_page();
                    p->ssd_sn = ssd_sn;
                    p->ssd_index = ssd_index;
                    p->pos = s.page_pos;
                    w.push_page(p);
                }
            }
            return true;
        }

        void
        free(uint64_t pos, uint64_t len) {
            capacity += len;
            auto s = slot{pos, len};
            if (current_slot.merge_able(s))
                current_slot.merge(s);
            else
                free_slots.emplace(s);
        }
    };
}

#endif //SIDER_KV_DATA_SLOTS_HH
