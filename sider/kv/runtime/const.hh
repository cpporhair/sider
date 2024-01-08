//

//

#ifndef SIDER_KV_RUNTIME_CONST_HH
#define SIDER_KV_RUNTIME_CONST_HH

namespace sider::kv::runtime {
    constexpr uint32_t data_page_size = 4096;
    constexpr uint32_t spdk_ring_size = 4096 * 1024;
    constexpr uint32_t spdk_dma_pool_size = 4096 * 128;
    constexpr uint32_t max_merge_pages = 4096;
    constexpr uint32_t ssd_block_size = 512;
    constexpr uint32_t max_cache_page = 1024;
}

#endif //SIDER_KV_RUNTIME_CONST_HH
