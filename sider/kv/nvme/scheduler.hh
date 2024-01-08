//

//

#ifndef SIDER_KV_NVME_SCHEDULER_HH
#define SIDER_KV_NVME_SCHEDULER_HH

#include "sider/kv/data/write_span.hh"

#include "./spdk_put.hh"
#include "./spdk_get.hh"
#include "./spdk_poll.hh"
#include "./putter.hh"
#include "./getter.hh"

namespace sider::kv::nvme {

    struct
    scheduler {
    private:
        friend struct _put_span::op<scheduler>;
        friend struct _get_data::op<scheduler>;
    private:
        uint32_t core;
        data::ssd* base_ssd;
        spdk_ring* put_span_req_queue;
        spdk_ring* get_data_req_queue;
        void* tmp[32];

        auto
        schedule(_get_data::req* req) {
            assert(0 < spdk_ring_enqueue(get_data_req_queue, (void**)&req, 1, nullptr));
        }

        auto
        schedule(_put_span::req* req) {
            assert(0 < spdk_ring_enqueue(put_span_req_queue, (void**)&req, 1, nullptr));
        }

        auto
        handle_push() {
            auto* qpr = base_ssd->chose_qpair(this->core);

            if (!qpr)
                return ;

            while(!qpr->busy()) {
                bool b_task = false;
                if (1 == spdk_ring_dequeue(put_span_req_queue, (void **) tmp, 1)) {
                    qpr->use();
                    b_task = true;
                    put_page((_put_span::req *) tmp[0], base_ssd->ns, qpr);
                }
                if (qpr->busy())
                    break;
                if (1 == spdk_ring_dequeue(get_data_req_queue, (void **) tmp, 1)) {
                    qpr->use();
                    b_task = true;
                    get_page((_get_data::req *) tmp[0], base_ssd->ns, qpr);
                }
                if (!b_task)
                    break;
            }
        }

        auto
        handle_poll() {
            for (auto *s: runtime::ssds.ssds)
                if(s->qpairs_by_core[this->core].poll_group)
                    poll(s->qpairs_by_core[this->core].poll_group);
        }
    public:
        scheduler(uint32_t c, data::ssd* base)
            : core(c)
            , base_ssd(base)
            , put_span_req_queue(spdk_ring_create(spdk_ring_type::SPDK_RING_TYPE_MP_SC, runtime::spdk_ring_size, -1))
            , get_data_req_queue(spdk_ring_create(spdk_ring_type::SPDK_RING_TYPE_MP_SC, runtime::spdk_ring_size, -1)){
        }
        auto
        get(data::data_page* page) {
            return _get_data::sender<scheduler>(this, page);
        }

        auto
        put(data::write_span& span) {
            return _put_span::sender<scheduler>(this, span);
        }

        inline
        uint32_t
        get_core() {
            return core;
        }

        void
        advance() {
            handle_push();
            handle_poll();
        }
    };
}

#endif //SIDER_KV_NVME_SCHEDULER_HH
