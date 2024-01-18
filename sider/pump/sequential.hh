//
//
//

#ifndef SIDER_PUMP_SEQUENTIAL_HH
#define SIDER_PUMP_SEQUENTIAL_HH


#include "./op_tuple_builder.hh"
#include "./submit.hh"
#include "./concurrent.hh"

#include "3rd/moodycamel/concurrentqueue.h"

namespace sider::pump {

    namespace _sequential {

        template <uint32_t pos, typename value_t>
        struct
        sink_pusher {
            constexpr static bool sequential_sink_pusher = true;
            constexpr static uint32_t parent_pusher_pos = pos;
            moodycamel::ConcurrentQueue<value_t>* queue;
        };

        template <typename value_t, typename stream_op_tuple_t>
        struct
        __ncp__(source_cache) {
            using value_type = value_t;
            constexpr static bool sequential_source_cache = true;
            moodycamel::ConcurrentQueue<value_t> queue;
            stream_op_tuple_t stream_op_tuple;
            bool ready = true;
            bool source_done = false;

            source_cache(stream_op_tuple_t&& ops)
                : stream_op_tuple(__fwd__(ops))
                , ready(true)
                , source_done(false)
                , queue(){
                __must_rval__(ops);
            }

            source_cache(source_cache&& o) noexcept
                : stream_op_tuple(__fwd__(o.stream_op_tuple))
                , ready(o.ready)
                , source_done(o.source_done)
                , queue(__fwd__(o.queue)){
                __must_rval__(o);
            }
        };
    }

    namespace pusher {
        template<uint32_t pos, typename scope_t>
        requires (pos < std::tuple_size_v<typename scope_t::element_type::op_tuple_type>)
        && (get_current_op_type_t<pos, scope_t>::sequential_source_cache)
        struct
        op_pusher<pos, scope_t> : op_pusher_base<pos, scope_t> {
            using queue_item_type = get_current_op_type_t<pos, scope_t>::value_type;

            template<uint32_t start_pos, typename context_t,typename this_scope_t>
            requires std::is_null_pointer_v<std::variant_alternative_t<2,queue_item_type>>
            static
            void
            do_push_next(context_t& context, this_scope_t& scope, queue_item_type& item) {
                switch(item.index()) {
                    case 0:
                        op_pusher<start_pos, this_scope_t>::push_skip(context, scope);
                        return;
                    case 1:
                        op_pusher<start_pos, this_scope_t>::push_exception(context, scope, __mov__(std::get<1>(item)));
                        return;
                    case 2:
                        op_pusher<start_pos, this_scope_t>::push_value(context, scope);
                        return;
                }
            }

            template<uint32_t start_pos, typename context_t,typename this_scope_t>
            static
            void
            do_push_next(context_t& context, this_scope_t& scope, queue_item_type& item) {
                switch(item.index_with_value()) {
                    case 0:
                        op_pusher<start_pos, this_scope_t>::push_skip(context, scope);
                        return;
                    case 1:
                        op_pusher<start_pos, this_scope_t>::push_exception(context, scope, __mov__(std::get<1>(item)));
                        return;
                    case 2:
                        op_pusher<start_pos, this_scope_t>::push_value(context, scope, __mov__(std::get<2>(item)));
                        return;
                }
            }

            template<typename context_t>
            static
            void
            poll_next(context_t& context, scope_t& scope) {
                auto& op = std::get<pos>(scope->get_op_tuple());
                typename __typ__(op.queue)::item_type item;
                if (!op.queue.try_dequeue(item)) {
                    op.ready = true;
                    if (op.source_done)
                        op_pusher<pos + 1, scope_t>::push_done(context, scope);
                }
                else {
                    do_push_next<pos + 1>(context, scope, item);
                }
            }

            template<typename context_t>
            static
            bool
            maybe_push(context_t& context, scope_t& scope) {
                auto& op = std::get<pos>(scope->get_op_tuple());
                if(!op.ready)
                    return false;
                op.ready = false;
                queue_item_type item;
                if (!op.queue.try_dequeue(item)) {
                    op.ready = true;
                    return false;
                }
                else {
                    auto new_scope = make_runtime_scope<runtime_scope_type::other>(
                        scope,
                        std::tuple_cat(
                            std::tie(op),
                            tuple_to_tie(op.stream_op_tuple),
                            std::tie(std::get<__typ__(op)::pos + 1>(find_stream_starter(scope)->get_op_tuple()))
                        )
                    );
                    do_push_next<1>(context, new_scope, item);
                    return true;
                }
            }

            template<typename context_t, typename value_t>
            static
            void
            push_value(context_t& context, scope_t& scope, value_t&& v) {
                auto& op = std::get<pos>(scope->get_op_tuple());
                op.queue.enqueue(queue_item_type(__fwd__(v)));
                maybe_push(context, scope);
            }

            template<typename context_t>
            static
            void
            push_value(context_t& context, scope_t& scope) {
                auto& op = std::get<pos>(scope->get_op_tuple());
                op.queue.enqueue(queue_item_type(nullptr));
                maybe_push(context, scope);
            }

            template<typename context_t>
            static
            void
            push_done(context_t& context, scope_t& scope) {
                auto& op = std::get<pos>(scope->get_op_tuple());
                op.source_done = true;
                if (!op.ready)
                    return;
                auto new_scope = make_runtime_scope<runtime_scope_type::other>(
                    scope,
                    std::tuple_cat(
                        std::tie(op),
                        tuple_to_tie(op.stream_op_tuple),
                        std::tie(std::get<__typ__(op)::pos + 1>(find_stream_starter(scope)->get_op_tuple()))
                    )
                );
                op_pusher<1, __typ__(new_scope)>::push_done(context, new_scope);
            }

            template<typename context_t>
            static inline
            void
            push_exception(context_t& context, scope_t& scope, std::exception_ptr e) {
                auto& op = std::get<pos>(scope->get_op_tuple());
                op.queue.enqueue(queue_item_type(__fwd__(e)));
                maybe_push(context, scope);
            }

            template<typename context_t>
            static inline
            void
            push_skip(context_t& context, scope_t& scope) {
                auto& op = std::get<pos>(scope->get_op_tuple());
                op.queue.enqueue(queue_item_type(std::monostate()));
                maybe_push(context, scope);
            }
        };
    }

    namespace _sequential {
        template <uint32_t pos, typename cache_vaule_t, typename parent_builder_t, typename stream_builder_t>
        struct
        __ncp__(sequential_starter_builder) {

            constexpr static uint32_t cur_pos = pos;

            parent_builder_t parent_builder;
            stream_builder_t stream_builder;

            sequential_starter_builder(parent_builder_t&& p, stream_builder_t&& s)
                : parent_builder(__fwd__(p))
                , stream_builder(__fwd__(s)){
                __must_rval__(p);
                __must_rval__(s);
            }

            sequential_starter_builder(sequential_starter_builder&& rhs)
                : parent_builder(__fwd__(rhs.parent_builder))
                , stream_builder(__fwd__(rhs.stream_tuple)){
            }

            template<typename pushed_op_t>
            auto
            push_back(pushed_op_t&& op){
                return
                    sequential_starter_builder
                        <
                            pos + 1,
                            cache_vaule_t,
                            __typ__(parent_builder),
                            __typ__(stream_builder.push_back(__fwd__(op)))
                        >
                        (
                            __mov__(parent_builder),
                            stream_builder.push_back(__fwd__(op))
                        );
            }

            auto
            make_source_cache() {
                return _sequential::source_cache<cache_vaule_t, __typ__(stream_builder.take())> ( __mov__(stream_builder.take()));
            }

            template<typename loop_end_op_op_t>
            auto
            push_ctrl_op(loop_end_op_op_t&& op) {
                if constexpr (std::tuple_size_v<__typ__(stream_builder.take())> == 0)
                    return parent_builder.push_ctrl_op(__fwd__(op));
                else
                    return parent_builder.push_ctrl_op(make_source_cache()).push_ctrl_op(__fwd__(op));
            }

            template<typename reduce_op_t>
            auto
            push_reduce_op(reduce_op_t&& op) {
                if constexpr (std::tuple_size_v<__typ__(stream_builder.take())> == 0)
                    return parent_builder.push_reduce_op(__fwd__(op));
                else
                    return parent_builder.push_ctrl_op(make_source_cache()).push_reduce_op(__fwd__(op));
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

            explicit
            sender(prev_t&& p)
                : prev(__fwd__(p)){
                __must_rval__(p);
            }

            sender(sender&& o)
                : prev(__fwd__(o.prev)){
            }

            template<typename context_t>
            requires typed::has_value_type<typed::compute_sender_type<context_t, prev_t>>
            auto
            connect() {
                using cache_value_type = std::variant<
                    std::monostate,
                    std::exception_ptr,
                    typename typed::compute_sender_type<context_t, prev_t>::value_type
                >;
                return sequential_starter_builder
                    <
                        1,
                        cache_value_type,
                        __typ__(prev.template connect<context_t>()),
                        builder::op_list_builder<0>
                    >
                    (
                        prev.template connect<context_t>(),
                        builder::op_list_builder<0>()
                    );
            }

            template<typename context_t>
            auto
            connect(){
                using cache_value_type = std::variant<std::monostate, std::exception_ptr, nullptr_t>;
                return sequential_starter_builder
                    <
                        1,
                        cache_value_type,
                        __typ__(prev.template connect<context_t>()),
                        builder::op_list_builder<0>
                    >
                    (
                        prev.template connect<context_t>(),
                        builder::op_list_builder<0>()
                    );
            }
        };

        struct
        fn {
            template <typename sender_t>
            constexpr
            decltype(auto)
            operator ()(sender_t&& s) const {
                __must_rval__(s);
                return sender<sender_t>{__fwd__(s)};
            }


            decltype(auto)
            operator ()() const {
                return bind_back<fn>(fn{});
            }
        };
    }

    namespace typed {
        template <typename context_t, typename prev_t>
        struct
        compute_sender_type<context_t, sider::pump::_sequential::sender<prev_t>> {
            using value_type = compute_sender_type<context_t, prev_t>::value_type;
        };
    }

    inline constexpr _sequential::fn sequential{};

    template <typename  bind_back_t>
    inline
    auto
    with_concurrent(bind_back_t&& b){
        return concurrent() >> __fwd__(b) >> sequential() ;
    }
}


#endif //SIDER_PUMP_SEQUENTIAL_HH
