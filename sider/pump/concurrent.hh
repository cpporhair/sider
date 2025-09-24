//
//
//

#ifndef PUMP_CONCURRENT_HH
#define PUMP_CONCURRENT_HH

#include <iostream>
#include <mutex>

#include "spdk/env.h"

#include "./op_tuple_builder.hh"
#include "./submit.hh"
#include "./then.hh"

namespace pump {

    namespace _concurrent {
        struct
        __ncp__(concurrent_counter) {
            std::atomic<uint64_t> counter;
            bool source_done;

        private:
            inline
            bool
            is_wait_eq_done(uint64_t test_value) {
                return (test_value & 0x00000000FFFFFFFF) == ((test_value >> 32) & 0x00000000FFFFFFFF);
            }
        public:
            concurrent_counter()
                : counter(0)
                , source_done(false){
            }

            inline
            void
            reset() {
                source_done=false;
                counter.store(0);
            }

            inline
            bool
            set_source_done(uint32_t count) {
                source_done = true;
                return add_wait(count);
            }

            inline
            bool
            add_wait(uint32_t count = 1) {
                uint64_t old_v, new_v = 0;
                bool res = 0;
                do {
                    old_v = counter.load();
                    new_v = old_v + ((uint64_t) count << 32);
                    res = is_wait_eq_done(new_v);
                } while (!counter.compare_exchange_weak(old_v, new_v, std::memory_order_relaxed));

                return source_done && res;
            }

            inline
            bool
            add_done(uint32_t count = 1){
                uint64_t old_v, new_v = 0;
                bool res = 0;
                do {
                    old_v = counter.load();
                    new_v = old_v + count;
                    res = is_wait_eq_done(new_v);
                } while (!counter.compare_exchange_weak(old_v, new_v, std::memory_order_relaxed));

                return source_done && res;;
            }
        };

        template <uint32_t pos, typename variant_value_t>
        struct
        concurrent_counter_wrapper {
            using variant_value_type = variant_value_t;
            constexpr static bool concurrent_counter_op = true;
            constexpr static uint32_t parent_pusher_pos = pos;
            concurrent_counter& impl;
            bool need_delete_scope;

            concurrent_counter_wrapper(concurrent_counter& c)
                : impl(c)
                , need_delete_scope(false){

            }

            concurrent_counter_wrapper(concurrent_counter_wrapper&& rhs)
                :impl(__fwd__(rhs.impl))
                , need_delete_scope(__fwd__(rhs.need_delete_scope)){
            }


            inline
            auto
            set_source_done(uint32_t all_count) {
                return impl.set_source_done(all_count);
            }

            inline
            bool
            add_done(){
                return impl.add_done();
            }
        };

        template <typename stream_op_tuple_t, typename variant_value_t>
        struct
        __ncp__(starter) {
            using variant_value_type = variant_value_t;
            constexpr static bool concurrent_starter_op = true;
            stream_op_tuple_t stream_op_tuple;
            concurrent_counter counter;
            std::atomic<uint32_t> count_of_values;

            std::atomic<uint64_t> now_pending;
            const uint64_t max_pending;

            starter(stream_op_tuple_t&& ops, uint64_t max_count)
                : stream_op_tuple(__fwd__(ops))
                , count_of_values(0)
                , now_pending(((uint64_t) max_count << 32) + 0)
                , max_pending(max_count){
                __must_rval__(ops);
            }

            starter(starter&& o) noexcept
                : stream_op_tuple(__fwd__(o.stream_op_tuple))
                , counter()
                , count_of_values(0)
                , now_pending(o.now_pending.load())
                , max_pending(o.max_pending){
                __must_rval__(o);
            }

            starter(const starter& o) noexcept
                : stream_op_tuple(o.stream_op_tuple)
                , counter()
                , count_of_values(0)
                , max_pending(o.max_pending)
                , now_pending(o.now_pending.load()){
            }

            inline
            void
            set_stream_done() {
                uint64_t old_v, new_v = 0;
                bool res = 0;
                do {
                    old_v = now_pending.load();
                    new_v = old_v | uint64_t(1);
                } while (!now_pending.compare_exchange_weak(old_v, new_v, std::memory_order_relaxed));
            }

            inline
            bool
            add_and_check_pending(uint32_t count = 1) {
                uint64_t old_v, new_v = 0;
                bool res = 0;
                do {
                    old_v = now_pending.load();
                    if (old_v & uint64_t(1))
                        return false;
                    new_v = old_v + ((uint64_t)count << 32);
                    res = (new_v >> 32) > max_pending;
                } while (!now_pending.compare_exchange_weak(old_v, new_v, std::memory_order_relaxed));

                return res;
            }
        };
    }

    namespace pusher {

        template<uint32_t pos, typename scope_t>
        requires (pos < std::tuple_size_v<typename scope_t::element_type::op_tuple_type>)
        && (get_current_op_type_t<pos, scope_t>::concurrent_counter_op)
        struct
        op_pusher<pos, scope_t> : op_pusher_base<pos, scope_t> {
            using variant_value_type = get_current_op_type_t<pos, scope_t>::variant_value_type;

            template<typename context_t, typename ...value_t>
            static inline
            void
            push_value(context_t& context, scope_t& scope, value_t&& ...v) {
                auto& op = std::get<pos>(scope->get_op_tuple());
                if (op.add_done()) {
                    op_pusher<pos + 1, scope_t>::counter_push_value(context, scope, __fwd__(v)...);
                    op_pusher<pos + 1, scope_t>::counter_push_done(context, scope);
                }
                else {
                    op.need_delete_scope = true;
                    op_pusher<pos + 1, scope_t>::counter_push_value(context, scope, __fwd__(v)...);
                }
            }

            template<typename context_t>
            static inline
            void
            push_value(context_t& context, scope_t& scope, variant_value_type&& v) {
                auto& op = std::get<pos>(scope->get_op_tuple());
                if (op.add_done()) {
                    op_pusher<pos + 1, scope_t>::counter_push_value(context, scope, __fwd__(v));
                    op_pusher<pos + 1, scope_t>::counter_push_done(context, scope);
                }
                else {
                    op.need_delete_scope = true;
                    op_pusher<pos + 1, scope_t>::counter_push_value(context, scope, __fwd__(v));
                }
            }

            template<typename context_t>
            static inline
            void
            push_exception(context_t& context, scope_t& scope, std::exception_ptr e) {
                auto& op = std::get<pos>(scope->get_op_tuple());
                if (op.add_done()) {
                    op_pusher<pos + 1, scope_t>::counter_push_exception(context, scope, e);
                    op_pusher<pos + 1, scope_t>::counter_push_done(context, scope);
                }
                else {
                    op.need_delete_scope = true;
                    op_pusher<pos + 1, scope_t>::counter_push_exception(context, scope, e);
                }
            }



            template<typename context_t>
            static inline
            void
            push_skip(context_t& context, scope_t& scope) {
                auto& op = std::get<pos>(scope->get_op_tuple());
                if (op.add_done()) {
                    //op_pusher<pos + 1, scope_t>::push_skip(context, scope);
                    op_pusher<pos + 1, scope_t>::counter_push_done(context, scope);
                }
                else {
                    op.need_delete_scope = true;
                    op_pusher<pos + 1, scope_t>::counter_push_skip(context, scope);
                }
            }


            template<typename context_t>
            static void
            set_source_done(context_t& context, scope_t& scope, uint32_t all_count) {
                auto& op = std::get<pos>(scope->get_op_tuple());;
                if (op.set_source_done(all_count))
                    op_pusher<pos + 1, scope_t>::counter_push_done(context, scope);
            }

            template<typename context_t>
            static void
            poll_next(context_t& context, scope_t& scope) {
            }
        };

        template<uint32_t pos, typename scope_t>
        requires (pos < std::tuple_size_v<typename scope_t::element_type::op_tuple_type>)
        && (get_current_op_type_t<pos, scope_t>::concurrent_starter_op)
        struct
        op_pusher<pos, scope_t> : op_pusher_base<pos, scope_t> {
            using variant_value_type = get_current_op_type_t<pos, scope_t>::variant_value_type;
            template <typename op_tuple_t>
            static inline
            auto
            make_new_scope(scope_t& scope, op_tuple_t&& t){
                return make_runtime_scope<runtime_scope_type::other>(
                    scope,
                    __fwd__(t)
                );
            }

            template<typename context_t>
            static void
            set_done(context_t& context, scope_t& scope) {
                auto& op = std::get<pos>(scope->get_op_tuple());
                op.set_stream_done();
            }

            /*
            template<typename context_t>
            static void
            poll_next(context_t& context, scope_t& scope) {
                auto& op = std::get<pos>(scope->get_op_tuple());
                std::lock_guard<std::recursive_mutex> ss(op.mu);
                if (op.stream_done)
                    return;
                op.now_pending--;
                __typ__(find_stream_starter(scope)) x = find_stream_starter(scope);
                op_pusher<0 , __typ__(x)>::poll_next(context, x);
            }
            */

            template<typename context_t, typename ...value_t>
            static inline
            void
            concurrent_impl(context_t& context, scope_t& scope) {
                auto& op = std::get<pos>(scope->get_op_tuple());

                if (op.add_and_check_pending())
                    return;
                else
                    op_pusher<0 , __typ__(find_stream_starter(scope))>::poll_next(context, find_stream_starter(scope));
            }

            template<typename context_t, typename ...value_t>
            static inline
            void
            push_value(context_t& context, scope_t& scope, value_t&& ...v) {
                auto& op = std::get<pos>(scope->get_op_tuple());
                auto new_scope = make_new_scope(
                    scope,
                    std::tuple_cat(
                        __typ__(op.stream_op_tuple)(op.stream_op_tuple),
                        std::make_tuple(_concurrent::concurrent_counter_wrapper<pos, variant_value_type>(op.counter)),
                        std::tie(op),
                        std::tie(std::get<__typ__(op)::pos + 1>(find_stream_starter(scope)->get_op_tuple()))
                    )
                );
                op.count_of_values++;
                op_pusher<0, __typ__(new_scope)>::push_value(context, new_scope, __fwd__(v)...);
                concurrent_impl(context, scope);

                //op_pusher<pos - 1, scope_t>::poll_next(context, scope);
            }



            template <typename context_t>
            static inline
            void
            push_done(context_t& context, scope_t& scope) {
                auto& op = std::get<pos>(scope->get_op_tuple());
                set_done(context, scope);
                auto new_scope = make_new_scope(
                    scope,
                    std::tuple_cat(
                        //__typ__(op.stream_op_tuple)(op.stream_op_tuple),
                        std::make_tuple(_concurrent::concurrent_counter_wrapper<pos, variant_value_type>(op.counter)),
                        std::tie(op),
                        std::tie(std::get<__typ__(op)::pos + 1>(find_stream_starter(scope)->get_op_tuple()))
                    )
                );
                op_pusher<0, __typ__(new_scope)>::set_source_done(context, new_scope, op.count_of_values);
            }

            template <typename context_t>
            static inline
            void
            push_exception(context_t& context, scope_t& scope, std::exception_ptr e) {
                auto& op = std::get<pos>(scope->get_op_tuple());
                auto new_scope = make_new_scope(
                    scope,
                    std::tuple_cat(
                        __typ__(op.stream_op_tuple)(op.stream_op_tuple),
                        std::make_tuple(_concurrent::concurrent_counter_wrapper<pos, variant_value_type>(op.counter)),
                        std::tie(op),
                        std::tie(std::get<__typ__(op)::pos + 1>(find_stream_starter(scope)->get_op_tuple()))
                    )
                );
                op.count_of_values++;
                op_pusher<0, __typ__(new_scope)>::push_exception(context, new_scope, e);
                concurrent_impl(context, scope);//op_pusher<pos - 1, scope_t>::poll_next(context, scope);
            }

            template <typename context_t>
            static inline
            void
            counter_push_done(context_t& context, scope_t& scope) {
                auto& op = std::get<pos>(scope->get_op_tuple());
                op_pusher<pos + 1, scope_t>::push_done(context, scope);
            }

            template<typename context_t, typename ...value_t>
            static inline
            void
            counter_push_value(context_t& context, scope_t& scope, value_t&& ...v) {
                op_pusher<pos + 1, scope_t>::push_value(context, scope, __fwd__(v)...);
            }

            template<typename context_t>
            static inline
            void
            counter_push_exception(context_t& context, scope_t& scope, std::exception_ptr e) {
                op_pusher<pos + 1, scope_t>::push_exception(context, scope, __fwd__(e));
            }

            template<typename context_t>
            static inline
            void
            counter_push_skip(context_t& context, scope_t& scope, std::exception_ptr e) {
                op_pusher<pos + 1, scope_t>::push_skip(context, scope, __fwd__(e));
            }
        };
    }

    namespace _concurrent {
        template <uint32_t pos, typename variant_value_t, typename parent_builder_t, typename stream_builder_t>
        struct
        __ncp__(concurrent_starter_builder) {

            constexpr static uint32_t cur_pos = pos;

            parent_builder_t parent_builder;
            stream_builder_t stream_builder;
            uint64_t max;

            concurrent_starter_builder(parent_builder_t&& p, stream_builder_t&& s, uint64_t max_count)
                : parent_builder(__fwd__(p))
                , stream_builder(__fwd__(s))
                , max(max_count){
                __must_rval__(p);
                __must_rval__(s);
            }

            concurrent_starter_builder(concurrent_starter_builder &&rhs)
                : parent_builder(__fwd__(rhs.parent_builder))
                , stream_builder(__fwd__(rhs.stream_builder))
                , max(rhs.max) {

            }

            template<typename pushed_op_t>
            auto
            push_back(pushed_op_t&& op) {
                return
                    concurrent_starter_builder
                        <
                            pos + 1,
                            variant_value_t,
                            __typ__(parent_builder),
                            __typ__(stream_builder.push_back(__fwd__(op)))
                        >
                        (
                            __mov__(parent_builder),
                            stream_builder.push_back(__fwd__(op)),
                            max
                        );
            }

            template<typename ctrl_op_t>
            auto
            push_ctrl_op(ctrl_op_t&& op) {
                return parent_builder.push_ctrl_op(
                        _concurrent::starter<
                            __typ__(stream_builder.take()),
                            variant_value_t
                        >(
                            __raw__(stream_builder.take()),
                            max
                        )
                    )
                    .push_ctrl_op(__fwd__(op));
            }

            template<typename reduce_op_t>
            auto
            push_reduce_op(reduce_op_t&& op) {
                return parent_builder.push_ctrl_op(
                        _concurrent::starter<
                            __typ__(stream_builder.take()),
                            variant_value_t
                        >(
                            __raw__(stream_builder.take()),
                            max
                        )
                    )
                    .push_reduce_op(__fwd__(op));
            }

            constexpr static uint32_t
            get_reduce_op_next_pos() {
                return parent_builder_t::get_reduce_op_next_pos();
            }
        };

        template <typename prev_t>
        struct
        __ncp__(sender) {
            using prev_type = prev_t;
            prev_t prev;
            uint64_t max;
            explicit
            sender(prev_t&& p, uint64_t max_count)
                : prev(__fwd__(p))
                , max(max_count){
                __must_rval__(p);
            }

            sender(sender&& o)
                : prev(__fwd__(o.prev))
                , max(o.max){
            }

            template<typename context_t>
            requires typed::has_value_type<typed::compute_sender_type<context_t, prev_t>>
            auto
            connect() {
                using variant_value_type = std::variant<
                    std::monostate,
                    std::exception_ptr,
                    typename typed::compute_sender_type<context_t, prev_t>::value_type
                >;

                return concurrent_starter_builder <
                    0,
                    variant_value_type,
                    __typ__(prev.template connect<context_t>()),
                    builder::op_list_builder<0>
                > (
                    prev.template connect<context_t>(),
                    builder::op_list_builder<0>(),
                    max
                );
            }

            template<typename context_t>
            auto
            connect(){
                using variant_value_type = std::variant<std::monostate, std::exception_ptr, nullptr_t>;
                return concurrent_starter_builder <
                    0,
                    variant_value_type,
                    __typ__(prev.template connect<context_t>()),
                    builder::op_list_builder<0>
                > (
                    prev.template connect<context_t>(),
                    builder::op_list_builder<0>(),
                    max
                );
            }
        };

        struct
        fn {
            template <typename sender_t>
            constexpr
            decltype(auto)
            operator ()(sender_t&& s, uint64_t max) const {
                __must_rval__(s);
                return _concurrent::sender<sender_t>{__fwd__(s), max};
            }


            decltype(auto)
            operator ()(uint64_t max = UINT64_MAX) const {
                return bind_back<fn, uint64_t>(fn{}, max);
            }
        };
    }

    namespace typed {
        template <typename context_t, typename prev_t>
        /*
        requires
        has_value_type<
            compute_sender_type<
                context_t,
                compute_sender_type<
                    context_t,
                    prev_t
                >
            >
        >
        */
        struct
        compute_sender_type<context_t, pump::_concurrent::sender<prev_t>> {
            using value_type = compute_sender_type<context_t, prev_t>::value_type;
            constexpr static bool has_value_type = true;
        };
    }

    inline constexpr _concurrent::fn concurrent{};
}

#endif //PUMP_CONCURRENT_HH
