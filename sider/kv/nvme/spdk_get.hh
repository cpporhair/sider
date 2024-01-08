//

//

#ifndef SIDER_KV_NVME_SPDK_GET_HH
#define SIDER_KV_NVME_SPDK_GET_HH

#include "sider/kv/runtime/dma.hh"

#include "./getter.hh"

namespace sider::kv::nvme {
    enum struct
    spdk_get_callback_status {
        done,
        failed,
        check_cpl
    };
    struct
    spdk_get_callback_arg {
        spdk_get_callback_status status;
        _get_data::req* raw_req;
        struct spdk_nvme_ns *ns;
        data::qpair* qp;
    };

    auto
    on_get_done(void* cb_arg, const spdk_nvme_cpl* cpl) {
        spdk_get_callback_arg* arg = (spdk_get_callback_arg*)cb_arg;
        _get_data::req *req = arg->raw_req;
        data::qpair *qp = arg->qp;

        delete arg;
        qp->no_more();
        req->cb(req->get_page());
        delete req;
    }

    auto
    get_page(_get_data::req* req, struct spdk_nvme_ns *ns, data::qpair* qp) {
        if (req->page->payload) [[unlikely]]
            return on_get_done(new spdk_get_callback_arg{spdk_get_callback_status::done, req, ns, qp}, nullptr);

        req->page->payload = data::make_data_page_payload();

        auto res = spdk_nvme_ns_cmd_read(
            ns,
            qp->impl,
            req->get_payload(),
            req->get_page_pos() / runtime::ssd_block_size,
            req->get_payload_size() / runtime::ssd_block_size,
            on_get_done,
            new spdk_get_callback_arg{spdk_get_callback_status::check_cpl, req, ns, qp},
            SPDK_NVME_IO_FLAGS_FORCE_UNIT_ACCESS
        );

        if(res < 0) [[unlikely]] {
            std::cout << res << std::endl;
            on_get_done(new spdk_get_callback_arg{spdk_get_callback_status::failed, req, ns, qp}, nullptr);
        }
    }
}

#endif //SIDER_KV_NVME_SPDK_GET_HH
