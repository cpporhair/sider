//

//

#ifndef SIDER_KV_DATA_CACHE_HH
#define SIDER_KV_DATA_CACHE_HH

namespace sider::kv::data {
    struct
    __ncp__(cached_data_file) {
        data_file* file;

        cached_data_file(data_file *f)
            : file(f) {
            file->add_ref();
        }

        cached_data_file(cached_data_file &&rhs)
            : file(__fwd__(rhs.file)) {
            rhs.file = nullptr;
        }

        ~cached_data_file() {
            if (!file)
                return;
            file->del_ref();
            file->free_payload();
        }
    };

    struct
    data_page_cache {
        std::list<cached_data_file> cache;

        auto
        maybe_pop() {
            if (cache.empty())
                return;

            auto iter = cache.begin();

            while (runtime::max_cache_page < cache.size()  && iter != cache.end()) {
                iter = cache.erase(iter);
            }
        }

        auto
        add_page(data_file* file) {
            file->add_flags(data::data_file_status::caching);
            cache.emplace_back(cached_data_file(file));
            maybe_pop();
        }
    };
}

#endif //SIDER_KV_DATA_CACHE_HH
