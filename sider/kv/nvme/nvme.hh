//

//

#ifndef MONISM_NVME_NVME_HH
#define MONISM_NVME_NVME_HH

#include "spdk/queue.h"

#include "./ssd.hh"
#include "./config.hh"
#include "./put.hh"

namespace sider::kv::nvme {

    template <typename page_list_t>
    struct
    nvme_impl {
        static
        bool
        probe_cb(
            void *cb_ctx,
            const spdk_nvme_transport_id *trid,
            spdk_nvme_ctrlr_opts *opts
        ){
            if(trid->trtype != SPDK_NVME_TRANSPORT_PCIE)
                return false;

            opts->num_io_queues = 32;
            opts->io_queue_size = UINT16_MAX;

            for(decltype(auto) e : global_ssd_config) {
                if(e.pcie_name == trid->traddr)
                    return true;
            }
            return false;
        }

        static
        auto
        attach_cb(
            void *cb_ctx,
            const struct spdk_nvme_transport_id *trid,
            struct spdk_nvme_ctrlr *ctrlr,
            const struct spdk_nvme_ctrlr_opts *opts
        ){
            int i = spdk_nvme_ctrlr_reset(ctrlr);
            if (i < 0) {
                std::cout << "spdk_nvme_ctrlr_reset return " << i << std::endl;
                exit(-1);
            }

            auto* this_sch = new scheduler<page_list_t>(new ssd(ctrlr, opts, trid));
            all_ssd<page_list_t>[this_sch->ssd_id()] = this_sch;
            all_ssd<page_list_t>[this_sch->ssd_id()]->ssd_id();
        }

        static
        auto
        init_all(const char* app_name, const char* core_mask) {
            spdk_nvme_transport_id trid{};
            spdk_nvme_trid_populate_transport(&trid, SPDK_NVME_TRANSPORT_PCIE);

            spdk_env_opts opts{};

            spdk_env_opts_init(&opts);
            opts.name = app_name;
            opts.core_mask = core_mask;
            if (spdk_env_init(&opts) < 0) {
                fprintf(stderr, "Unable to initialize SPDK in spdk_env_init\n");
                return 1;
            }

            all_ssd<page_list_t> = new scheduler<page_list_t>* [global_ssd_config.size()];

            if (spdk_nvme_probe(&trid, nullptr, probe_cb, attach_cb, nullptr) < 0) {
                fprintf(stderr, "Unable to initialize SPDK in spdk_nvme_probe\n");
                return 1;
            }

            global_dma.init();
            return 0;
        }

        static
        auto
        forward_coro(uint32_t core, uint32_t& stop_flag)
        -> coro::empty_yields {
            while (stop_flag != 0) {
                for (int i = 0; i < global_ssd_config.size(); ++i) {
                    all_ssd<page_list_t>[i]->forward(core);
                }
                co_yield {};
            }
            co_return {};
        }
    };


}

#endif //MONISM_NVME_NVME_HH
