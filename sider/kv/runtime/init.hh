//

//

#ifndef SIDER_KV_RUNTIME_INIT_HH
#define SIDER_KV_RUNTIME_INIT_HH

#include "./nvme.hh"
#include "./scheduler_objects.hh"

namespace sider::kv::runtime {
    auto
    init_ssd() {
        for (uint32_t i = 0; i < config.io.ssds.size(); ++i) {
            auto* s = new data::ssd();
            s->config = config.io.ssds[i];
            s->index_on_config = i;
            ssds.ssds.push_back(s);
        }

        spdk_nvme_transport_id trid{};
        spdk_nvme_trid_populate_transport(&trid, SPDK_NVME_TRANSPORT_PCIE);

        if (spdk_nvme_probe(&trid, nullptr, probe_cb, attach_cb, nullptr) < 0)
            throw data::init_env_failed("Unable to initialize SPDK in spdk_nvme_probe");

        for (auto *ssd : ssds.ssds) {
            while (!ssd->un_allocated_qpairs.empty()) {
                for (auto c: ssd->config.used_core) {
                    if (!ssd->un_allocated_qpairs.empty()) {
                        auto *qpr = *ssd->un_allocated_qpairs.begin();
                        ssd->qpairs_by_core[c].qpairs.emplace_back(new data::qpair(qpr, c));
                        ssd->un_allocated_qpairs.pop_front();
                    }
                }
            }
        }
    }

    auto
    init_allocator() {
        uint32_t ssd_index = 0;
        for (auto *ssd : ssds.ssds) {
            ssd->allocator = new data::slot_manager(ssd->sn, ssd_index++,
                                                    ssd->sector_size * ssd->max_sector / runtime::data_page_size);
        }
    }

    auto
    init_indexes() {
        for (auto c : runtime::config.index.used_core) {
            all_index.by_core[c] = new data::b_tree;
            all_index.list.emplace_back(all_index.by_core[c]);
        }
    }

    auto
    init_schedulers() {

        for (auto *ssd : ssds.ssds) {
            for(auto c : ssd->config.used_core) {
                if (ssd->qpairs_by_core[c].qpairs.empty())
                    continue;
                else
                    ssd->qpairs_by_core[c].poll_group = spdk_nvme_poll_group_create(nullptr, nullptr);
                for (auto *qpr : ssd->qpairs_by_core[c].qpairs) {
                    spdk_nvme_poll_group_add(ssd->qpairs_by_core[c].poll_group, qpr->impl);
                    spdk_nvme_ctrlr_connect_io_qpair(ssd->ctrlr, qpr->impl);
                }
            }
        }

        for (auto *ssd : ssds.ssds) {
            nvme_schedulers_per_ssd.push_back(nvme_scheduler_list(ssd->sn));
            for (auto c: ssd->config.used_core) {
                auto *sche = new nvme::scheduler(c, ssd);
                nvme_schedulers_per_ssd[nvme_schedulers_per_ssd.size() - 1]
                    .by_core[c] = sche;
                nvme_schedulers_per_ssd[nvme_schedulers_per_ssd.size() - 1]
                    .list.push_back(sche);
            }
        }


        for (auto c : runtime::config.batch.used_core) {
            batch_schedulers.by_core[c] = new batch::scheduler(c);
            batch_schedulers.list.emplace_back(batch_schedulers.by_core[c]);
        }

        for (auto c : runtime::config.index.used_core) {
            index_schedulers.by_core[c] = new index::scheduler(c, all_index.by_core[c]);
            index_schedulers.list.emplace_back(index_schedulers.by_core[c]);
        }

        for (auto *ssd : ssds.ssds) {
            fs_schedulers.by_core[ssd->config.fs_core] = new fs::scheduler(ssd->config.fs_core, ssd);
            fs_schedulers.list.emplace_back(fs_schedulers.by_core[ssd->config.fs_core]);
        }

        for (auto c = spdk_env_get_first_core(); c != UINT32_MAX; c = spdk_env_get_next_core(c)){
            task_schedulers.by_core[c] = new task::scheduler(c);
            task_schedulers.list.emplace_back(task_schedulers.by_core[c]);
        }
    }

    auto
    init_core_mask() {
        std::stringstream ss;
        ss << std::hex << runtime::config.compute_core_mask();
        std::cout << ss.str() << std::endl;
        return ss.str();
    }

    auto
    init_proc_env() {
        std::string core_mask_string = init_core_mask();

        spdk_env_opts opts{};
        spdk_env_opts_init(&opts);
        opts.name = "monism";
        opts.core_mask =core_mask_string.c_str();

        if (spdk_env_init(&opts) < 0)
            throw data::init_env_failed("Unable to initialize SPDK in spdk_env_init");
        global_page_dma.init();
    }

    auto
    init_config(std::string path) {
        return runtime::read_config_from_file(config, path);
    }

    auto
    make_config(std::string path) {
        db_config c;
        runtime::read_config_from_file(c, path);
        return c;
    }
}

#endif //SIDER_KV_RUNTIME_INIT_HH
