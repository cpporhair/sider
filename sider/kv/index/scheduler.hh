//

//

#ifndef SIDER_KV_INDEX_SCHEDULER_HH
#define SIDER_KV_INDEX_SCHEDULER_HH

#include "spdk/env.h"

#include "sider/kv/data/b_tree.hh"
#include "sider/kv/data/cache.hh"
#include "sider/kv/data/exceptions.hh"
#include "sider/kv/runtime/index.hh"

namespace sider::kv::index {
    namespace _update {
        struct
        req {
            data::data_file* f;
            std::move_only_function<void()> cb;
        };
        template <typename scheduler_t>
        struct
        op {
            constexpr static bool index_update_op = true;
            scheduler_t* scheduler;
            data::data_file* file;
            op(scheduler_t* s, data::data_file* f)
                : scheduler(s)
                , file(f) {
            }

            op(op&& rhs)
                : scheduler(rhs.scheduler)
                , file(rhs.file) {
            }

            template<uint32_t pos, typename context_t, typename scope_t>
            auto
            start(context_t &context, scope_t &scope){
                scheduler->schedule(
                    new (scheduler->update_req_pool.malloc_req()) req {
                        file,
                        [context = context, scope = scope]() mutable {
                            runtime::g_task_info.index_count++;

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
            data::data_file* file;

            sender(scheduler_t* s, data::data_file* f)
                : scheduler(s)
                , file(f){
            }

            sender(sender &&rhs)
                : scheduler(rhs.s)
                , file(rhs.file) {
            }

            inline
            auto
            make_op(){
                return op<scheduler_t>(scheduler, file);
            }

            template<typename context_t>
            auto
            connect() {
                return pump::builder::op_list_builder<0>().push_back(make_op());
            }
        };
    }

    namespace _cached {
        struct
        req {
            data::data_file* f;
            std::move_only_function<void()> cb;
        };
        template <typename scheduler_t>
        struct
        op {
            constexpr static bool index_cached_op = true;
            scheduler_t* scheduler;
            data::data_file* file;
            op(scheduler_t* s, data::data_file* f)
                : scheduler(s)
                , file(f) {
            }

            op(op&& rhs)
                : scheduler(rhs.scheduler)
                , file(rhs.file) {
            }

            template<uint32_t pos, typename context_t, typename scope_t>
            auto
            start(context_t &context, scope_t &scope){
                scheduler->schedule(
                    new (scheduler->cached_req_pool.malloc_req()) req {
                        file,
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
            data::data_file* file;

            sender(scheduler_t* s, data::data_file* f)
                : scheduler(s)
                , file(f){
            }

            sender(sender &&rhs)
                : scheduler(rhs.s)
                , file(rhs.file) {
            }

            inline
            auto
            make_op(){
                return op<scheduler_t>(scheduler, file);
            }

            template<typename context_t>
            auto
            connect() {
                return pump::builder::op_list_builder<0>().push_back(make_op());
            }
        };
    }

    namespace _b_tree {
        struct
        req {
            std::move_only_function<void(data::b_tree*)> cb;
        };

        template<typename scheduler_t>
        struct
        op {
            constexpr static bool btree_getter_op = true;
            scheduler_t* scheduler;

            op(scheduler_t* s)
                : scheduler(s) {
            }

            op(op &&rhs)
                : scheduler(rhs.scheduler) {
            }

            template<uint32_t pos, typename context_t, typename scope_t>
            auto
            start(context_t &context, scope_t &scope){
                scheduler->schedule(
                    new (scheduler->b_tree_req_pool.malloc_req()) req {
                        [context = context, scope = scope](data::b_tree* root) mutable {
                            pump::pusher::op_pusher<pos + 1, scope_t>::push_value(
                                context,
                                scope,
                                root
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

            sender(scheduler_t* s)
                : scheduler(s) {
            }

            sender(sender &&rhs)
                : scheduler(rhs.s) {
            }

            inline
            auto
            make_op(){
                return op<scheduler_t>(scheduler);
            }

            template<typename context_t>
            auto
            connect() {
                return pump::builder::op_list_builder<0>().push_back(make_op());
            }
        };
    }

    namespace _getter {

        struct
        not_fround_res {
        };

        struct
        pager_reader_res {
            const data::slice* key;
            data::version* ver;
            data::key_value kv;
        };

        struct
        pager_waiter_res {
            data::key_value kv;
        };

        using result_type = std::variant<pager_reader_res *, pager_waiter_res *, not_fround_res, data::read_page_failed>;

        struct
        req {
            data::slice* k;
            uint64_t read_sn;
            uint64_t free_sn;
            std::move_only_function<void(result_type&&)> cb;
        };

        template <typename scheduler_t>
        struct
        op {
            constexpr static bool index_getter_op = true;
            scheduler_t *scheduler;
            data::slice* key;
            uint64_t read_sn;
            uint64_t free_sn;

            op(scheduler_t *s, data::slice* k, uint64_t rsn, uint64_t fsn)
                : scheduler(s)
                , key(k)
                , read_sn(rsn)
                , free_sn(fsn)  {
            }

            op(op &&rhs)
                : scheduler(rhs.scheduler)
                , key(rhs.key)
                , read_sn(rhs.read_sn)
                , free_sn(rhs.free_sn) {
            }

            template<uint32_t pos, typename context_t, typename scope_t>
            auto
            start(context_t &context, scope_t &scope){
                scheduler->schedule(
                    new (scheduler->getter_req_pool.malloc_req()) req {
                        key,
                        read_sn,
                        free_sn,
                        [context = context, scope = scope](result_type&& v) mutable {
                            std::visit(
                                [&context, &scope](auto&& v) {
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

        template <typename scheduler_t>
        struct
        sender {
            scheduler_t *scheduler;
            data::slice* key;
            uint64_t read_sn;
            uint64_t free_sn;

            sender(scheduler_t *s, data::slice* k, uint64_t rsn, uint64_t fsn)
                : scheduler(s)
                , key(k)
                , read_sn(rsn)
                , free_sn(fsn)  {
            }

            sender(sender &&rhs)
                : scheduler(rhs.scheduler)
                , key(rhs.key)
                , read_sn(rhs.read_sn)
                , free_sn(rhs.free_sn) {
            }

            inline
            auto
            make_op(){
                return op<scheduler_t>(scheduler, key, read_sn, free_sn);
            }

            template<typename context_t>
            auto
            connect() {
                return pump::builder::op_list_builder<0>().push_back(make_op());
            }
        };
    }

    namespace _tasker {
        struct
        req {
            std::move_only_function<void()> cb;
        };

        template <typename scheduler_t>
        struct
        op {
            constexpr static bool index_tasker_op = true;
            scheduler_t *scheduler;

            op(scheduler_t* s)
                : scheduler(s) {
            }

            op(op &&rhs)
                : scheduler(rhs.scheduler) {
            }

            template<uint32_t pos, typename context_t, typename scope_t>
            auto
            start(context_t &context, scope_t &scope){
                scheduler->schedule(
                    new (scheduler->tasker_req_pool.malloc_req()) req {
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

        template <typename scheduler_t>
        struct
        sender {
            constexpr static bool index_tasker_op = true;
            scheduler_t *scheduler;

            sender(scheduler_t *s)
                : scheduler(s) {
            }

            sender(sender &&rhs)
                : scheduler(rhs.scheduler) {
            }

            inline
            auto
            make_op(){
                return op<scheduler_t>(scheduler);
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
        friend struct _update::op<scheduler>;
        friend struct _cached::op<scheduler>;
        friend struct _getter::op<scheduler>;
        friend struct _tasker::op<scheduler>;
        friend struct _b_tree::op<scheduler>;
    private:
        uint32_t core;
        constexpr static uint32_t handle_req_per_call = 4096;
        spdk_ring* update_req_queue;
        spdk_ring* cached_req_queue;
        spdk_ring* getter_req_queue;
        spdk_ring* tasker_req_queue;
        spdk_ring* b_tree_req_queue;

        runtime::req_pool<sizeof(_update::req)> update_req_pool;
        runtime::req_pool<sizeof(_cached::req)> cached_req_pool;
        runtime::req_pool<sizeof(_getter::req)> getter_req_pool;
        runtime::req_pool<sizeof(_tasker::req)> tasker_req_pool;
        runtime::req_pool<sizeof(_b_tree::req)> b_tree_req_pool;

        data::data_page_cache cache;
        data::b_tree* indexes;
        void* tmp[handle_req_per_call];

        auto
        schedule(_update::req* req) {
            assert(0 < spdk_ring_enqueue(update_req_queue, (void **) &req, 1, nullptr));
        }

        auto
        schedule(_cached::req* req) {
            assert(0 < spdk_ring_enqueue(cached_req_queue, (void **) &req, 1, nullptr));
        }

        auto
        schedule(_getter::req* req) {
            runtime::g_task_info.gen_count++;
            assert(0 < spdk_ring_enqueue(getter_req_queue, (void **) &req, 1, nullptr));
        }

        auto
        schedule(_tasker::req* req) {
            assert(0 < spdk_ring_enqueue(tasker_req_queue, (void **) &req, 1, nullptr));
        }

        auto
        schedule(_b_tree::req* req) {
            assert(0 < spdk_ring_enqueue(b_tree_req_queue, (void **) &req, 1, nullptr));
        }

        void
        handle_update() {
            uint32_t size = spdk_ring_dequeue(update_req_queue, tmp, handle_req_per_call);
            for (uint32_t i = 0; i < size; ++i) {
                auto* req = (_update::req *) tmp[i];
                indexes->update(req->f);
                req->cb();
                update_req_pool.free_req((void*)req);
            }
        }

        void
        handle_cached() {
            uint32_t size = spdk_ring_dequeue(cached_req_queue, tmp, handle_req_per_call);
            for (uint32_t i = 0; i < size; ++i) {
                auto* req = (_cached::req *) tmp[i];
                cache.add_page(req->f);
                req->f->add_flags(data::data_file_status::opening);
                req->cb();
                cached_req_pool.free_req((void*)req);
            }
        }

        void
        handle_getter() {
            uint32_t size = spdk_ring_dequeue(getter_req_queue, tmp, handle_req_per_call);
            for (uint32_t i = 0; i < size; ++i) {
                auto* req = (_getter::req *) tmp[i];
                auto* key = req->k;
                auto* idx = indexes->find(key);
                runtime::g_task_info.publish += 1;
                if(!idx) {
                    req->cb(_getter::not_fround_res{});
                    getter_req_pool.free_req((void*)req);
                    continue;
                }

                auto* ver = idx->get_version(req->read_sn);
                if (!ver) {
                    req->cb(_getter::not_fround_res{});
                    getter_req_pool.free_req((void*)req);
                    continue;
                }

                if (ver->file->has_payload()) {
                    req->cb(new _getter::pager_waiter_res{data::key_value(ver->file)});
                    getter_req_pool.free_req((void*)req);
                    continue;
                }

                if (ver->page_waiters.size() == 0) {
                    req->cb(new _getter::pager_reader_res{idx->key, ver, data::key_value(ver->file)});
                    getter_req_pool.free_req((void*)req);
                }
                else {
                    ver->page_waiters.emplace_back((void *) req);
                }
            }
        }

        void
        handle_b_tree() {
            uint32_t size = spdk_ring_dequeue(b_tree_req_queue, tmp, handle_req_per_call);
            for (uint32_t i = 0; i < size; ++i) {
                auto* req = (_b_tree::req *) tmp[i];
                req->cb(indexes);
                b_tree_req_pool.free_req((void*)req);
            }
        }

        void
        handle_tasker() {
            uint32_t size = spdk_ring_dequeue(tasker_req_queue, tmp, handle_req_per_call);
            for (uint32_t i = 0; i < size; ++i) {
                auto* req = (_tasker::req *) tmp[i];
                req->cb();
                tasker_req_pool.free_req((void*)req);
            }
        }

    public:
        scheduler(uint32_t c, data::b_tree* root)
            : core(c)
            , indexes(root)
            , update_req_queue(spdk_ring_create(spdk_ring_type::SPDK_RING_TYPE_MP_SC, runtime::spdk_ring_size, -1))
            , cached_req_queue(spdk_ring_create(spdk_ring_type::SPDK_RING_TYPE_MP_SC, runtime::spdk_ring_size, -1))
            , getter_req_queue(spdk_ring_create(spdk_ring_type::SPDK_RING_TYPE_MP_SC, runtime::spdk_ring_size, -1))
            , tasker_req_queue(spdk_ring_create(spdk_ring_type::SPDK_RING_TYPE_MP_SC, runtime::spdk_ring_size, -1))
            , b_tree_req_queue(spdk_ring_create(spdk_ring_type::SPDK_RING_TYPE_MP_SC, runtime::spdk_ring_size, -1))
            , update_req_pool((std::string("index_update_req_pool") + std::to_string(c)).c_str())
            , cached_req_pool((std::string("index_cached_req_pool") + std::to_string(c)).c_str())
            , getter_req_pool((std::string("index_getter_req_pool") + std::to_string(c)).c_str())
            , tasker_req_pool((std::string("index_tasker_req_pool") + std::to_string(c)).c_str())
            , b_tree_req_pool((std::string("index_b_tree_req_pool") + std::to_string(c)).c_str()) {
        }

        auto
        update(data::data_file* f) {
            return _update::sender<scheduler>(this, f);
        }

        auto
        cache_file(data::data_file* f) {
            return _cached::sender<scheduler>(this, f);
        }

        auto
        get(data::slice* key, uint64_t read_sn, uint64_t free_sn) {
            return _getter::sender<scheduler>(this, key, read_sn, free_sn);
        }

        auto
        get_b_tree() {
            return _b_tree::sender<scheduler>(this);
        }

        auto
        via() {
            return _tasker::sender<scheduler>(this);
        }

        inline
        uint32_t
        get_core() {
            return core;
        }

        void
        advance() {
            handle_update();
            handle_cached();
            handle_getter();
            handle_tasker();
            handle_b_tree();
        }
    };
}

namespace pump::pusher {
    template<uint32_t pos, typename scope_t>
    requires (pos < std::tuple_size_v<typename scope_t::element_type::op_tuple_type>)
    && (get_current_op_type_t<pos, scope_t>::index_update_op)
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
    && (get_current_op_type_t<pos, scope_t>::index_cached_op)
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
    && (get_current_op_type_t<pos, scope_t>::index_getter_op)
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
    && (get_current_op_type_t<pos, scope_t>::btree_getter_op)
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
    && (get_current_op_type_t<pos, scope_t>::index_tasker_op)
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
    compute_sender_type<context_t, sider::kv::index::_update::sender<scheduler_t>> {
        constexpr static bool multi_values= false;
    };

    template <typename context_t, typename scheduler_t>
    struct
    compute_sender_type<context_t, sider::kv::index::_cached::sender<scheduler_t>> {
        constexpr static bool multi_values= false;
    };

    template <typename context_t, typename scheduler_t>
    struct
    compute_sender_type<context_t, sider::kv::index::_b_tree::sender<scheduler_t>> {
        using value_type = sider::kv::data::b_tree*;
        constexpr static bool multi_values= false;
    };

    template <typename context_t, typename scheduler_t>
    struct
    compute_sender_type<context_t, sider::kv::index::_getter::sender<scheduler_t>> {
        using value_type = sider::kv::index::_getter::result_type;
        constexpr static bool multi_values= false;
    };

    template <typename context_t, typename scheduler_t>
    struct
    compute_sender_type<context_t, sider::kv::index::_tasker::sender<scheduler_t>> {
        constexpr static bool multi_values= false;
    };
}

#endif //SIDER_KV_INDEX_SCHEDULER_HH
