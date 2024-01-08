//

//

#ifndef MONISM_NVME_SSD_HH
#define MONISM_NVME_SSD_HH

#include "sider/util/ring.hh"

#include "qpair.hh"
#include "./config.hh"

namespace sider::kv::nvme {

    struct
    ssd {
        spdk_nvme_ctrlr*        ctrlr;
        spdk_nvme_ns*           ns;
        uint32_t                id;
        uint32_t                sector_size;
        uint64_t                max_sector;
        ssd_config              config;
        std::vector<qpair*>*    qp_each_core;

        auto
        init_qpair(const spdk_nvme_ctrlr_opts* ctrl_opts) {
            spdk_nvme_io_qpair_opts opts{};
            spdk_nvme_ctrlr_get_default_io_qpair_opts(ctrlr, &opts, sizeof(spdk_nvme_io_qpair_opts));
            opts.delay_cmd_submit = true;
            opts.create_only = true;

            auto* tmp_q = utils::make_ring<spdk_nvme_qpair*>(1024);

            for (uint32_t i = 0; i < ctrl_opts->num_io_queues; ++i) {
                push_ring(tmp_q, spdk_nvme_ctrlr_alloc_io_qpair(ctrlr, &opts, sizeof(spdk_nvme_io_qpair_opts)));
            }

            while(!tmp_q->empty()) {
                for (uint32_t c = spdk_env_get_first_core(); c != UINT32_MAX && !tmp_q->empty(); c = spdk_env_get_next_core(c)) {
                    if (std::find(config.qpair_on_cores.begin(), config.qpair_on_cores.end(), c)
                        != config.qpair_on_cores.end()) {
                        qp_each_core[c].push_back(new qpair{poll_ring(tmp_q), c, 0});
                    }
                }
            }

            assert(tmp_q->empty());
            utils::free_ring(tmp_q);
        }

        ssd(spdk_nvme_ctrlr* c, const spdk_nvme_ctrlr_opts* opts, const spdk_nvme_transport_id *trid) {
            this->ctrlr = c;
            this->ns = spdk_nvme_ctrlr_get_ns(ctrlr,spdk_nvme_ctrlr_get_first_active_ns(ctrlr));
            auto l = spdk_nvme_ns_get_md_size(this->ns);
            this->sector_size = spdk_nvme_ns_get_sector_size(this->ns);
            this->max_sector = spdk_nvme_ns_get_num_sectors(this->ns);
            this->qp_each_core = new std::vector<qpair *>[max_core_count];

            for(decltype(auto) e : global_ssd_config) {
                if(e.pcie_name == trid->traddr)
                    this->config = e;
            }
            this->init_qpair(opts);
        }
    };
}

#endif //MONISM_NVME_SSD_HH
