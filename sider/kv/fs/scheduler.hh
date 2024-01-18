//

//

#ifndef SIDER_KV_FS_SCHEDULER_HH
#define SIDER_KV_FS_SCHEDULER_HH

#include "spdk/env.h"

#include "sider/kv/data/write_span.hh"
#include "sider/kv/data/exceptions.hh"
#include "sider/kv/data/slots.hh"
#include "sider/kv/runtime/config.hh"
#include "sider/kv/runtime/nvme.hh"


namespace sider::kv::fs {
    namespace _allocate {

        struct leader_res ;
        struct follower_res ;
        struct failed_res ;

        struct
        req {
            data::batch* batch;
            std::move_only_function<void(std::variant<leader_res*, follower_res, failed_res>)> cb;
        };

        struct
        leader_res {
            data::write_span_list span_list;
            std::vector<req*> followers_req;
        };

        struct
        follower_res {

        };

        struct
        failed_res {
        };

        template <typename scheduler_t>
        struct
        op {
            constexpr static bool fs_allocate_op = true;
            scheduler_t* scheduler;
            data::batch* batch;

            op(scheduler_t* s, data::batch* b)
                : scheduler(s)
                , batch(b) {
            }

            op(op&& rhs)
                : scheduler(rhs.scheduler)
                , batch(rhs.batch) {
            }

            template<uint32_t pos, typename context_t, typename scope_t>
            auto
            start(context_t &context, scope_t &scope) {
                return scheduler->schedule(
                    new req{
                        batch,
                        [context = context, scope = scope](std::variant<leader_res *, follower_res, failed_res>&& v) mutable {
                            if(scope.use_count() == 0) {
                                std::cout << "AAAAAAAAAAAAAA" << std::endl;
                            }
                            std::visit(
                                [&context, &scope](auto&& v){
                                    pump::pusher::op_pusher<pos + 1, scope_t>::push_value(
                                        context,
                                        scope,
                                        __fwd__(v)
                                    );
                                },
                                __fwd__(v)
                            );
                        }
                    }
                );
            }
        };

        template<typename scheduler_t>
        struct
        sender {
            scheduler_t* scheduler;
            data::batch* batch;

            sender(scheduler_t* s, data::batch* b)
                : scheduler(s)
                , batch(b){
            }

            sender(sender &&rhs)
                : scheduler(rhs.s)
                , batch(rhs.batch) {
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
        };
    }

    namespace _freepage {

        struct
        req {
            data::write_span& span;
            std::move_only_function<void()> cb;
        };

        template <typename scheduler_t>
        struct
        op {
            constexpr static bool fs_free_op = true;
            scheduler_t* scheduler;
            data::write_span& span;

            op(scheduler_t* s, data::write_span& b)
                : scheduler(s)
                , span(b) {
            }

            op(op&& rhs)
                : scheduler(rhs.scheduler)
                , span(rhs.span) {
            }

            template<uint32_t pos, typename context_t, typename scope_t>
            auto
            start(context_t &context, scope_t &scope) {
                return scheduler->schedule(
                    new req{
                        span,
                        [context = context, scope = scope]() mutable {
                            pump::pusher::op_pusher<pos + 1, scope_t>::push_value(
                                context,
                                scope
                            );
                        }
                    }
                );
            }
        };

        template<typename scheduler_t>
        struct
        sender {
            scheduler_t* scheduler;
            data::write_span& span;

            sender(scheduler_t* s, data::write_span& b)
                : scheduler(s)
                , span(b){
            }

            sender(sender &&rhs)
                : scheduler(rhs.s)
                , span(rhs.span) {
            }

            inline
            auto
            make_op(){
                return op<scheduler_t>(scheduler, span);
            }

            template<typename context_t>
            auto
            connect() {
                return pump::builder::op_list_builder<0>().push_back(make_op());
            }
        };
    }

    namespace _metadata {
        struct leader_res ;
        struct follower_res ;
        struct failed_res ;

        struct
        req {
            _allocate::leader_res* prev_allocate_res;
            std::move_only_function<void(std::variant<leader_res*, follower_res, failed_res>)> cb;
        };

        struct
        leader_res {
            data::write_span_list span_list;
            std::vector<req*> followers_req;
        };

        struct
        follower_res {

        };

        struct
        failed_res {
        };

        template <typename scheduler_t>
        struct
        op {
            constexpr static bool fs_allocate_op = true;
            scheduler_t* scheduler;
            _allocate::leader_res* prev_allocate_res;

            op(scheduler_t* s, _allocate::leader_res* b)
                : scheduler(s)
                , prev_allocate_res(b) {
            }

            op(op&& rhs)
                : scheduler(rhs.scheduler)
                , prev_allocate_res(rhs.prev_allocate_res) {
            }

            template<uint32_t pos, typename context_t, typename scope_t>
            auto
            start(context_t &context, scope_t &scope) {
                return scheduler->schedule(
                    new req{
                        prev_allocate_res,
                        [context = context, scope = scope](std::variant<leader_res *, follower_res, failed_res>&& v) mutable {
                            std::visit(
                                [&context, &scope](auto&& v){
                                    pump::pusher::op_pusher<pos + 1, scope_t>::push_value(
                                        context,
                                        scope,
                                        __fwd__(v)
                                    );
                                },
                                __fwd__(v)
                            );
                        }
                    }
                );
            }
        };

        template<typename scheduler_t>
        struct
        sender {
            scheduler_t* scheduler;
            _allocate::leader_res* prev_allocate_res;

            sender(scheduler_t* s, _allocate::leader_res* b)
                : scheduler(s)
                , prev_allocate_res(b){
            }

            sender(sender &&rhs)
                : scheduler(rhs.s)
                , prev_allocate_res(rhs.prev_allocate_res) {
            }

            inline
            auto
            make_op(){
                return op<scheduler_t>(scheduler, prev_allocate_res);
            }

            template<typename context_t>
            auto
            connect() {
                return pump::builder::op_list_builder<0>().push_back(make_op());
            }
        };
    }

    struct
    scheduler {
    private:
        friend struct _allocate::op<scheduler>;
        friend struct _freepage::op<scheduler>;
        friend struct _metadata::op<scheduler>;
    private:
        uint32_t core;
        data::ssd* base_ssd;
        spdk_ring* allocate_req_queue;
        spdk_ring* freepage_req_queue;
        spdk_ring* metadata_req_queue;
        uint32_t cursor;
        void* tmp[4096];

        auto
        schedule(_allocate::req* req) {
            assert(0 < spdk_ring_enqueue(allocate_req_queue, (void**)&req, 1, nullptr));
        }

        auto
        schedule(_freepage::req* req) {
            assert(0 < spdk_ring_enqueue(freepage_req_queue, (void**)&req, 1, nullptr));
        }

        auto
        schedule(_metadata::req* req) {
            assert(0 < spdk_ring_enqueue(metadata_req_queue, (void**)&req, 1, nullptr));
        }

        void
        handle_allocate() {
            _allocate::leader_res *res = nullptr;
            _allocate::req* ldr = nullptr;

            while(
                (((res == nullptr) || (res->span_list.page_count() <= runtime::max_merge_pages)) &&
                    (1 == spdk_ring_dequeue(allocate_req_queue, (void **) tmp, 1)))
                ) {
                if (res == nullptr)
                    res = new _allocate::leader_res();
                runtime::g_task_info.fs_count ++;
                auto* req = (_allocate::req*)tmp[0];
                if (base_ssd->allocator->allocate(res->span_list, req->batch)) {
                    if (ldr)
                        res->followers_req.push_back(req);
                    else
                        ldr = req;
                }
                else {
                    std::for_each(req->batch->cache.begin(), req->batch->cache.end(), [](data::key_value& kv) {
                        std::for_each(kv.file->pages.begin(), kv.file->pages.end(), [](data::data_page *p) { p->set_error(); });
                    });
                    req->cb(_allocate::failed_res{});
                }
            }


            if(!ldr)
                return;

            ldr->cb(res);
        }

        void
        handle_free() {
            uint32_t size = spdk_ring_dequeue(freepage_req_queue, (void **) tmp, 1);

            if (size > 0) {
                _freepage::req *req = (_freepage::req *) tmp[0];
                base_ssd->allocator->free(req->span.pos, req->span.pages.size());
                delete (_freepage::req *) tmp[0];
            }
        }

        void
        handle_meta() {
            uint32_t size = spdk_ring_dequeue(metadata_req_queue, (void **) tmp, 1);
            for (uint32_t i = 0; i < size; ++i) {
                _metadata::req *req = (_metadata::req *) tmp[i];
                req->cb(_metadata::follower_res());
                delete req;
            }
        }

    public:
        scheduler(uint32_t c, data::ssd* ssd)
            : core(c)
            , base_ssd(ssd)
            , allocate_req_queue(spdk_ring_create(spdk_ring_type::SPDK_RING_TYPE_MP_SC, runtime::spdk_ring_size, -1))
            , freepage_req_queue(spdk_ring_create(spdk_ring_type::SPDK_RING_TYPE_MP_SC, runtime::spdk_ring_size, -1))
            , metadata_req_queue(spdk_ring_create(spdk_ring_type::SPDK_RING_TYPE_MP_SC, runtime::spdk_ring_size, -1))
            , cursor(0) {
        }

        auto
        allocate_data_page(data::batch* batch){
            return _allocate::sender(this, batch);
        }

        auto
        allocate_meta_page(_allocate::leader_res* res){
            return _metadata::sender(this, res);
        }

        auto
        free(data::write_span& span) {
            return _freepage::sender(this, span);
        }

        inline
        uint32_t
        get_core() {
            return core;
        }

        void
        advance() {
            handle_allocate();
            handle_free();
            handle_meta();
        }
    };

}

namespace sider::pump::pusher {
    template<uint32_t pos, typename scope_t>
    requires (pos < std::tuple_size_v<typename scope_t::element_type::op_tuple_type>)
    && (get_current_op_type_t<pos, scope_t>::fs_allocate_op)
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
    && (get_current_op_type_t<pos, scope_t>::fs_free_op)
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

namespace sider::pump::typed {
    template <typename context_t, typename scheduler_t>
    struct
    compute_sender_type<context_t, sider::kv::fs::_allocate::sender<scheduler_t>> {
        using value_type = sider::kv::fs::_allocate::leader_res *;//std::variant<sider::kv::fs::_allocate::leader_res *, sider::kv::fs::_allocate::follower_res>;
        constexpr static bool multi_values= false;
        constexpr static bool has_value_type = true;
    };

    template <typename context_t, typename scheduler_t>
    struct
    compute_sender_type<context_t, sider::kv::fs::_metadata::sender<scheduler_t>> {
        using value_type = sider::kv::fs::_metadata::leader_res *;//std::variant<sider::kv::fs::_allocate::leader_res *, sider::kv::fs::_allocate::follower_res>;
        constexpr static bool multi_values= false;
        constexpr static bool has_value_type = true;
    };

    template <typename context_t, typename scheduler_t>
    struct
    compute_sender_type<context_t, sider::kv::fs::_freepage::sender<scheduler_t>> {
        constexpr static bool multi_values= false;
        constexpr static bool has_value_type = false;
    };
}

#endif //SIDER_KV_FS_SCHEDULER_HH
