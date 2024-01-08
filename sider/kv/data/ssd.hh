//

//

#ifndef SIDER_KV_DATA_SSD_HH
#define SIDER_KV_DATA_SSD_HH

#include <array>
#include "spdk/nvme.h"

#include "sider/util/ring.hh"

#include "./slots.hh"

namespace sider::kv::data {
    struct
    qpair {
        spdk_nvme_qpair* impl;
        uint32_t core;
        uint32_t used;
        bool locked;

        qpair(spdk_nvme_qpair* raw, uint32_t c)
            : impl(raw)
            , core(c)
            , used(0)
            , locked(false){
        }

        inline
        void
        use() {
            ++used;
        }

        inline
        void
        no_more() {
            used--;
        }

        inline
        bool
        busy(){
            return 1024 < used;
        }
    };

    struct
    qpair_groups {
        std::vector<qpair*> qpairs;
        spdk_nvme_poll_group* poll_group;
        uint32_t cursor = 0;

        inline
        uint32_t
        forward_cursor() {
            ++cursor;
            if (qpairs.size() <= cursor)
                cursor = 0;
            return cursor;
        }

        qpair*
        chose_qpair() {
            uint32_t old = cursor;
            do {
                if (!qpairs[cursor]->busy())
                    return qpairs[cursor];
                forward_cursor();
            }
            while (old != cursor);
            return nullptr;
        }
    };

    struct
    ssd {
        runtime::nvme_config        config;
        const char*                 sn;
        spdk_nvme_ctrlr*            ctrlr;
        spdk_nvme_ns*               ns;
        uint32_t                    index_on_config;
        uint32_t                    sector_size;
        uint64_t                    max_sector;
        std::vector<qpair_groups>   qpairs_by_core;
        std::list<spdk_nvme_qpair*> un_allocated_qpairs;
        slot_manager*               allocator;

        ssd()
            : qpairs_by_core(std::vector<qpair_groups>(std::thread::hardware_concurrency()))
            , allocator(nullptr){
        }

        qpair*
        chose_qpair(uint32_t core) {
            return qpairs_by_core[core].chose_qpair();
        }

        auto
        create_qpairs(const spdk_nvme_ctrlr_opts* ctrl_opts) {
            spdk_nvme_io_qpair_opts opts{};
            spdk_nvme_ctrlr_get_default_io_qpair_opts(ctrlr, &opts, sizeof(spdk_nvme_io_qpair_opts));
            opts.delay_cmd_submit = true;
            opts.create_only = true;

            for (uint32_t i = 0; i < ctrl_opts->num_io_queues; ++i)
                un_allocated_qpairs.emplace_back(
                    spdk_nvme_ctrlr_alloc_io_qpair(ctrlr, &opts, sizeof(spdk_nvme_io_qpair_opts)));
        }
    };
}

#endif //SIDER_KV_DATA_SSD_HH
