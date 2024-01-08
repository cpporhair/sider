//

//

#ifndef SIDER_KV_DATA_WRITE_SPAN_HH
#define SIDER_KV_DATA_WRITE_SPAN_HH

namespace sider::kv::data {
    struct
    __ncp__(write_span) {
        const char* ssd_sn;
        uint32_t ssd_index;
        uint64_t pos;
        std::vector<data_page*> pages;

        write_span(data_page *page)
            : ssd_sn(page->ssd_sn)
            , pos(page->pos)
            , ssd_index(page->ssd_index)
            , pages{page} {

        }

        inline
        bool
        merge_able(data_page* p) {
            return (ssd_sn == p->ssd_sn)
                   && (ssd_index == p->ssd_index)
                   && ((pos + pages.size()) == p->pos);
        }

        bool
        try_merge(data_page* p) {
            if (!merge_able(p))
                return false;
            pages.push_back(p);
            return true;
        }

        inline
        void
        set_wrote() {
            for (auto* p : pages)
                p->set_wrote(true);
        }

        inline
        void
        set_error() {
            for (auto* p : pages)
                p->set_error();
        }

        inline
        bool
        all_wrote() {
            return std::all_of(pages.begin(), pages.end(),[](auto* p){return p->is_wrote();});
        }
    };

    struct
    write_span_list {
        uint32_t page_size = 0;
        std::list<write_span> spans;

        void
        push_page(data_page* page) {
            page_size++;

            for(auto &e : spans)
                if(e.try_merge(page))
                    return;
            spans.emplace_back(page);
        }

        uint32_t
        page_count() {
            return page_size;
        }
    };
};

#endif //SIDER_KV_DATA_WRITE_SPAN_HH
