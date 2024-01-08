//

//

#ifndef SIDER_KV_IMPL_DMA_HH
#define SIDER_KV_IMPL_DMA_HH

#include "spdk/env.h"

#include "./const.hh"
#include "./config.hh"

namespace sider::kv::runtime {

    template <size_t ele_size>
    struct
    req_pool {
        static constexpr size_t size = ele_size;
        spdk_mempool* pool;

        req_pool(const char * name) {
            init(name);
        }

        void
        init(const char * name) {
            pool = spdk_mempool_create(name, spdk_dma_pool_size, size, 4096, SPDK_ENV_SOCKET_ID_ANY);
            for (uint32_t i = 0; i < 1024; ++i) {
                spdk_mempool_put(pool, malloc(size));
            }
        }

        void*
        malloc_req() {
            auto* res = spdk_mempool_get(pool);
            if (!res)[[unlikely]]{
                g_task_info.page_count++;
                res = malloc(size);
            }

            return res;
        }

        void
        free_req(void* dma) {
            spdk_mempool_put(pool, dma);
        }
    };

    struct
    page_dma {
        spdk_mempool* global_pool;

        void
        init(spdk_mempool* pool) {
            for (uint32_t i = 0; i < 1024; ++i) {
                spdk_mempool_put(pool, spdk_dma_malloc(data_page_size, data_page_size, nullptr));
            }
        }

        void
        init() {
            global_pool = spdk_mempool_create("global", spdk_dma_pool_size, data_page_size, 4096, SPDK_ENV_SOCKET_ID_ANY);
            init(global_pool);
        }

        void*
        malloc() {
            auto* res = spdk_mempool_get(global_pool);
            if (!res)[[unlikely]]{
                g_task_info.page_count++;
                res = spdk_dma_malloc(data_page_size, data_page_size, nullptr);
            }

            return res;
        }

        void
        free(void* dma) {
            spdk_mempool_put(global_pool, dma);
        }
    };

    page_dma global_page_dma;

    std::atomic<uint64_t> malloc_count = 0;

    inline
    auto
    malloc_page() {
        malloc_count++;
        return global_page_dma.malloc();
    }

    inline
    auto
    free_page(void* dma) {
        if (dma == nullptr)
            return;
        malloc_count--;
        return global_page_dma.free(dma);
    }
}
#endif //SIDER_KV_IMPL_DMA_HH
