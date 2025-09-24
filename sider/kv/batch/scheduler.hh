//

//

#ifndef SIDER_KV_BATCH_SCHEDULER_HH
#define SIDER_KV_BATCH_SCHEDULER_HH

#include "spdk/env.h"

#include "./publish_req.hh"
#include "./request_task.hh"
#include "./snapshot_manager.hh"

namespace sider::kv::batch {
    namespace _publish {
        template <typename scheduler_t>
        struct
        __ncp__(op) {
            constexpr static bool batch_publish_op = true;

            scheduler_t* scheduler;
            data::batch* batch;

            op(scheduler_t* s, data::batch* b)
                : scheduler(s)
                , batch(b) {
            }

            op(op&& rhs) noexcept
                : scheduler(rhs.scheduler)
                , batch(rhs.batch) {

            }

            template<uint32_t pos, typename context_t, typename scope_t>
            auto
            start(context_t& context, scope_t& scope) {
                scheduler->schedule(
                    new publish_req(
                        batch,
                        [context = context, scope = scope]()mutable {
                            pump::pusher::op_pusher<pos + 1, scope_t>::push_value(context, scope);
                        }
                    )
                );
            }
        };

        template <typename scheduler_t>
        struct
        sender {
        public:
            sender(scheduler_t* schd, data::batch* b) noexcept
                : scheduler(schd)
                , batch(b) {
            }

            inline
            auto
            make_op(){
                return op<scheduler_t>(scheduler, batch);
            }

            template<typename context_t>
            auto
            connect() {
                return pump::builder::op_list_builder<0>().push_back(make_op());
            }

            scheduler_t* scheduler;
            data::batch* batch;
        };
    }

    namespace _request_read {
        template <typename scheduler_t>
        struct
        __ncp__(op) {
            constexpr static bool batch_request_read_op = true;
            scheduler_t* scheduler;
            data::batch* batch;

            op(scheduler_t* s, data::batch* b)
                : scheduler(s)
                , batch(b){
            }

            op(op&& rhs)
                : scheduler(__fwd__(rhs.scheduler))
                , batch(__fwd__(rhs.batch)){
            }

            template<uint32_t pos, typename context_t, typename scope_t>
            auto
            start(context_t& context, scope_t& scope) {
                scheduler->schedule(
                    new request_get_id_task(
                        [context = context, batch = batch, scope = scope](data::snapshot* get, uint64_t last_free_sn)mutable {
                            batch->get_snapshot = get;
                            batch->last_free_sn = last_free_sn;
                            pump::pusher::op_pusher<pos + 1, scope_t>::push_value(
                                context,
                                scope
                            );
                        }
                    )
                );
            }
        };

        template <typename scheduler_t>
        struct
        sender {
        public:
            sender(scheduler_t* schd, data::batch* b) noexcept
                : scheduler(schd)
                , batch(b){
            }

            sender(sender&& rhs) noexcept
                : scheduler(rhs.scheduler)
                , batch(rhs.batch){
            }

            inline
            auto
            make_op(){
                return op<scheduler_t>(scheduler, batch);
            }

            template<typename context_t>
            auto
            connect() {
                return pump::builder::op_list_builder<0>().push_back(make_op());
            }

            scheduler_t* scheduler;
            data::batch* batch;
        };
    }

    namespace _allocate_put_id {
        template <typename scheduler_t>
        struct
        __ncp__(op) {
            constexpr static bool batch_request_put_id_op = true;
            scheduler_t* scheduler;
            data::batch* batch;

            op(scheduler_t* s, data::batch* b)
                : scheduler(s)
                , batch(b){
            }

            op(op&& rhs)
                : scheduler(__fwd__(rhs.scheduler))
                , batch(__fwd__(rhs.batch)){
            }

            template<uint32_t pos, typename context_t, typename scope_t>
            auto
            start(context_t& context, scope_t& scope) {
                scheduler->schedule(
                    new request_put_id_task(
                        [context = context, batch = batch, scope = scope](data::snapshot* snp, uint64_t last_free_sn)mutable {
                            batch->put_snapshot = snp;
                            batch->last_free_sn = last_free_sn;
                            batch->update_put_sn();
                            pump::pusher::op_pusher<pos + 1, scope_t>::push_value(
                                context,
                                scope
                            );
                        }
                    )
                );
            }
        };

        template <typename scheduler_t>
        struct
        sender {
        public:
            sender(scheduler_t* schd, data::batch* b) noexcept
                : scheduler(schd)
                , batch(b){
            }

            sender(sender&& rhs) noexcept
                : scheduler(rhs.scheduler)
                , batch(rhs.batch){
            }

            inline
            auto
            make_op(){
                return op<scheduler_t>(scheduler, batch);
            }

            template<typename context_t>
            auto
            connect() {
                return pump::builder::op_list_builder<0>().push_back(make_op());
            }

            scheduler_t* scheduler;
            data::batch* batch;
        };
    }

    struct
    scheduler {
        friend struct _publish::op<scheduler>;
        friend struct _request_read::op<scheduler>;
        friend struct _allocate_put_id::op<scheduler>;
    private:
        uint32_t core;
        spdk_ring* publish_task_queue;
        spdk_ring* request_read_task_queue;
        spdk_ring* allocate_put_id_task_queue;
        snapshot_manager snapshots;
        void* tmp[4096];
    private:

        auto
        handle_publish() {
            size_t size = 0;
            size = spdk_ring_dequeue(publish_task_queue, tmp, 4096);
            for (uint32_t i = 0; i < size; ++i) {
                snapshots.push_in_unpublished_queue((publish_req*)tmp[i]);
            }
            snapshots.try_publish();
        }

        auto
        handle_request_read_snapshot() {
            size_t size = 0;
            size = spdk_ring_dequeue(request_read_task_queue, tmp, 4096);
            for (uint32_t i = 0; i < size; ++i) {
                runtime::g_task_info.start_batch_count++;
                request_get_id_task *req = (request_get_id_task *) tmp[i];
                snapshots.request_read(req);
                delete req;
            }
        }

        auto
        handle_allocate_put_id() {
            size_t size = 0;
            size = spdk_ring_dequeue(allocate_put_id_task_queue, tmp, 4096);
            for (uint32_t i = 0; i < size; ++i) {
                auto* req = (request_put_id_task*)tmp[i];
                snapshots.allocate_put_id(req);
                delete req;
            }
        }

        auto
        handle_gc() {
            snapshots.gc();
        }

    private:
        void
        schedule(publish_req* req) {
            //update_maximum(runtime::g_task_info.publish, req->batch->put_snapshot->get_serial_number());
            assert(0 < spdk_ring_enqueue(publish_task_queue, (void **) (&req), 1, nullptr));
        }

        void
        schedule(request_get_id_task* req) {
            assert(0 < spdk_ring_enqueue(request_read_task_queue, (void **) (&req), 1, nullptr));
        }

        void
        schedule(request_put_id_task* req) {
            assert(0 < spdk_ring_enqueue(allocate_put_id_task_queue, (void **) (&req), 1, nullptr));
        }

    public:
        scheduler(uint32_t c)
            : core(c)
            , publish_task_queue(spdk_ring_create(spdk_ring_type::SPDK_RING_TYPE_MP_SC, runtime::spdk_ring_size * 10, 0))
            , request_read_task_queue(spdk_ring_create(spdk_ring_type::SPDK_RING_TYPE_MP_SC, runtime::spdk_ring_size , 0))
            , allocate_put_id_task_queue(spdk_ring_create(spdk_ring_type::SPDK_RING_TYPE_MP_SC, runtime::spdk_ring_size , 0)){
        };
    public:
        inline
        auto
        publish(data::batch* b) {
            return _publish::sender(this, b);
        }

        inline
        auto
        start_batch(data::batch* b) {
            return _request_read::sender{this, b};
        }

        inline
        auto
        allocate_put_snapshot(data::batch* b) {
            return _allocate_put_id::sender{this, b};
        }

        inline
        uint32_t
        get_core() {
            return core;
        }

        void
        advance() {
            handle_publish();
            handle_request_read_snapshot();
            handle_allocate_put_id();
            handle_gc();
        }
    };

}

namespace pump::pusher {
    template<uint32_t pos, typename scope_t>
    requires (pos < std::tuple_size_v<typename scope_t::element_type::op_tuple_type>)
    && (get_current_op_type_t<pos, scope_t>::batch_publish_op)
    struct
    op_pusher<pos, scope_t> : op_pusher_base<pos, scope_t> {
        template<typename context_t>
        static inline
        void
        push_value(context_t& context, scope_t& scope) {
            std::get<pos>(scope->get_op_tuple()).template start<pos>(context, scope);
        }
    };

    template<uint32_t pos, typename scope_t>
    requires (pos < std::tuple_size_v<typename scope_t::element_type::op_tuple_type>)
    && (get_current_op_type_t<pos, scope_t>::batch_request_read_op)
    struct
    op_pusher<pos, scope_t> : op_pusher_base<pos, scope_t> {
        template<typename context_t>
        static inline
        void
        push_value(context_t& context, scope_t& scope) {
            std::get<pos>(scope->get_op_tuple()).template start<pos>(context, scope);
        }
    };

    template<uint32_t pos, typename scope_t>
    requires (pos < std::tuple_size_v<typename scope_t::element_type::op_tuple_type>)
    && (get_current_op_type_t<pos, scope_t>::batch_request_put_id_op)
    struct
    op_pusher<pos, scope_t> : op_pusher_base<pos, scope_t> {
        template<typename context_t>
        static inline
        void
        push_value(context_t& context, scope_t& scope) {
            std::get<pos>(scope->get_op_tuple()).template start<pos>(context, scope);
        }
    };
}

namespace pump::typed {
    template <typename context_t, typename scheduler_t>
    struct
    compute_sender_type<context_t, sider::kv::batch::_request_read::sender<scheduler_t>> {
        constexpr static bool multi_values= false;
    };

    template <typename context_t, typename scheduler_t>
    struct
    compute_sender_type<context_t, sider::kv::batch::_publish::sender<scheduler_t>> {
        constexpr static bool multi_values= false;
    };

    template <typename context_t, typename scheduler_t>
    struct
    compute_sender_type<context_t, sider::kv::batch::_allocate_put_id::sender<scheduler_t>> {
        constexpr static bool multi_values= false;
    };
}

#endif //SIDER_KV_BATCH_SCHEDULER_HH
