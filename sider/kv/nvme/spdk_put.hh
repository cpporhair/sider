//

//

#ifndef SIDER_KV_NVME_SPDK_PUT_HH
#define SIDER_KV_NVME_SPDK_PUT_HH

#include "sider/kv/data/ssd.hh"
#include "./putter.hh"

namespace sider::kv::nvme {

    struct
    spdk_put_callback_arg {
        _put_span::req* raw_req;
        struct spdk_nvme_ns *ns;
        data::qpair* qp;
    };

    auto
    on_put_done(void *cb_arg, const spdk_nvme_cpl *cpl) {
        auto *arg = (spdk_put_callback_arg *) cb_arg;
        auto *req = arg->raw_req;
        auto *qpr = arg->qp;

        delete arg;

        qpr->no_more();

        if (!cpl || spdk_nvme_cpl_is_error(cpl))
            req->span.set_error();
        else
            req->span.set_wrote();
        req->cb(req->span);
        delete req;
    }

    void
    on_put_request_reset_sgl(void *cb_arg, uint32_t sgl_offset){
        auto *arg = (spdk_put_callback_arg *) cb_arg;

        auto* req = arg->raw_req;
        req->sgl_pos_by_page = sgl_offset / runtime::data_page_size;
    }

    int
    on_put_request_next_sge(void *cb_arg, void **address, uint32_t *length) {
        auto *arg = (spdk_put_callback_arg *) cb_arg;
        auto* req = arg->raw_req;
        *address = req->span.pages[req->sgl_pos_by_page]->payload;
        *length = runtime::data_page_size;
        return 0;
    }

    auto
    put_page(_put_span::req* req, struct spdk_nvme_ns *ns, data::qpair* qp) {
        runtime::g_task_info.nv_count += req->span.pages.size();
        auto res = spdk_nvme_ns_cmd_writev(
            ns,
            qp->impl,
            req->span.pos * runtime::data_page_size / runtime::ssd_block_size,
            req->span.pages.size() * runtime::data_page_size / runtime::ssd_block_size,
            on_put_done,
            new spdk_put_callback_arg{req, ns, qp},
            SPDK_NVME_IO_FLAGS_FORCE_UNIT_ACCESS,
            on_put_request_reset_sgl,
            on_put_request_next_sge
        );

        if(res < 0) [[unlikely]] {
            std::cout << res << std::endl;
            on_put_done(new spdk_put_callback_arg{req, ns, qp}, nullptr);
        }
    }
}

#endif //SIDER_KV_NVME_SPDK_PUT_HH
