//
//
//

#ifndef PUMP_WHEN_ALL_HH
#define PUMP_WHEN_ALL_HH

#include <variant>

#include "./any_context.hh"
#include "./compute_sender_type.hh"
#include "./get_context.hh"
#include "./flat.hh"
#include "./op_pusher.hh"
#include "./op_tuple_builder.hh"
#include "./helper.hh"

namespace pump{
    template <typename ...value_t>
    struct
    when_all_res {
    };

    template <typename value_t>
    struct
    when_all_res<value_t> {
        using type = std::variant<std::monostate, std::exception_ptr, value_t>;
    };

    template <>
    struct
    when_all_res<void> {
        using type = std::variant<std::monostate, std::exception_ptr, nullptr_t>;
    };

    template <typename context_t, typename sender_t>
    struct
    when_all_compute {
        using type = void;
    };

    template <typename context_t, typename sender_t>
    requires pump::typed::has_value_type<pump::typed::compute_sender_type<context_t,sender_t>>
    struct
    when_all_compute<context_t, sender_t> {
        using type = pump::typed::compute_sender_type<context_t,sender_t>::value_type;
    };

    namespace _when_all {
        template <uint32_t pos,typename parent_context_t, typename parent_scope_t>
        struct
        parent_pusher_status {
            constexpr static uint32_t parent_pusher_pos = pos;
            parent_context_t context;
            parent_scope_t scope;
        };

        template <typename ...value_t>
        struct
        value_collector {
            std::tuple<value_t...> values;

            template <uint32_t index>
            inline
            auto
            set_value() {
                std::get<index>(values).template emplace<2>(nullptr);
            }

            template <uint32_t index, typename arg_t>
            inline
            auto
            set_value(arg_t&& arg) {
                std::get<index>(values).template emplace<2>(__fwd__(arg));
            }

            template <uint32_t index>
            void
            set_error(std::exception_ptr e){
                std::get<index>(values).template emplace<1>(e);
            }

            template <uint32_t index>
            void
            set_done(){
                std::get<index>(values).template emplace<0>(std::monostate{});
            }

            template <uint32_t index>
            void
            set_skip(){
                std::get<index>(values).template emplace<0>(std::monostate{});
            }
        };

        template <uint32_t max, typename value_collector_t, typename parent_pusher_status_t>
        struct
        __ncp__(collector_wrapper) {
            value_collector_t value_collector;
            std::atomic<int> done_count;
            parent_pusher_status_t parent_pusher_status;

            collector_wrapper(value_collector_t&& v, parent_pusher_status_t&& p)
                : value_collector(__fwd__(v))
                , parent_pusher_status(__fwd__(p))
                , done_count(max){
            }

            collector_wrapper(collector_wrapper &&) = delete;

            auto
            on_set() {
                if (done_count.fetch_sub(1) == 1) {
                    std::apply(
                        [this](auto& ...args){
                            pusher::op_pusher<
                                parent_pusher_status_t::parent_pusher_pos,
                                __typ__(parent_pusher_status.scope)
                            >::
                            push_value(
                                parent_pusher_status.context,
                                parent_pusher_status.scope,
                                __mov__(args)...
                            );
                        },
                        value_collector.values
                    );
                }
            }

            template <uint32_t index, typename ...value_t>
            inline
            auto
            set_value(value_t&& ...v) {
                value_collector.template set_value<index,value_t...>(__fwd__(v)...);
                on_set();
            }

            template <uint32_t index>
            void
            set_error(std::exception_ptr e){
                value_collector.template set_error<index>(e);
                on_set();
            }

            template <uint32_t index>
            void
            set_done(){
                value_collector.template set_done<index>();
                on_set();
            }

            template <uint32_t index>
            void
            set_skip(){
                value_collector.template set_skip<index>();
                on_set();
            }
        };

        template <uint32_t index, typename collector_wrapper_t>
        struct
        __ncp__(reducer) {
            constexpr static uint32_t pos = index;
            constexpr static bool when_all_reducer = true;
            collector_wrapper_t* wrapper;

            reducer(collector_wrapper_t* c)
                : wrapper(c){
            }

            reducer(reducer&& rhs)
                : wrapper(rhs.wrapper){
            }

            template <typename ...value_t>
            auto
            set_value(value_t&& ...value) {
                wrapper->template set_value<index>(__fwd__(value)...);
            }

            void
            set_error(std::exception_ptr e){
                wrapper->template set_error<index>(e);
            }

            void
            set_done(){
                wrapper->template set_done<index>();
            }

            void
            set_skip(){
                wrapper->template set_skip<index>();
            }
        };

        template <typename result_t, typename ...op_list_t>
        struct
        __ncp__(starter) {
            constexpr static bool when_all_starter = true;
            std::tuple<op_list_t...> all_inner_op_list;

            starter(op_list_t&& ...opt)
                : all_inner_op_list(std::make_tuple(__fwd__(opt)...)) {
            }

            starter(starter&& rhs)
                : all_inner_op_list(__fwd__(rhs.all_inner_op_list)){
            }

            starter(const starter& rhs)
                : all_inner_op_list(rhs.all_inner_op_list){
            }

            template <uint32_t index, typename context_t, typename scope_t, typename collector_t>
            inline
            void
            submit_senders(context_t& context,scope_t& scope, collector_t* collector){

                auto new_scope = make_runtime_scope<runtime_scope_type::other>(
                    scope,
                    push_back_to_op_list(
                        tuple_to_tie(std::get<index>(all_inner_op_list)),
                        reducer<index, collector_t>(collector)
                    )
                );

                pusher::op_pusher<0, __typ__(new_scope)>::push_value(context, new_scope);
                if constexpr ((index+1) < sizeof...(op_list_t))
                    submit_senders<index + 1>(context, scope, collector);
            }

            template<typename context_t, typename scope_t, typename parent_pusher_status_t>
            auto
            start(context_t& context, scope_t& scope, parent_pusher_status_t&& s){
                auto collector = new collector_wrapper<sizeof...(op_list_t), result_t, parent_pusher_status_t>(
                    result_t(),
                    __fwd__(s)
                );
                submit_senders<0>(context, scope, collector);
            }
        };

        template <typename prev_t, typename ...sender_t>
        struct
        __ncp__(sender) {

            using prev_type = prev_t;

            std::tuple<sender_t...> inner_senders;
            prev_t prev;

            sender(prev_t&& p, sender_t&& ...inner_sender)
                : prev(__fwd__(p))
                , inner_senders(std::make_tuple(__fwd__(inner_sender)...)){
            }

            sender(sender&& o)noexcept
                : prev(__fwd__(o.prev))
                , inner_senders(__fwd__(o.inner_senders)){
            }

            template<uint32_t index, typename context_t>
            inline
            auto
            connect_inner_sender() {
                if constexpr (size_t(index+1) == sizeof...(sender_t))
                    return std::make_tuple(std::get<index>(inner_senders).template connect<context_t>().take());
                else
                    return std::tuple_cat(
                        std::make_tuple(std::get<index>(inner_senders).template connect<context_t>().take()),
                        connect_inner_sender<index + 1, context_t>()
                    );
            }

            template<typename context_t>
            auto
            connect() {
                using result_t = value_collector<
                    typename when_all_res<
                        typename when_all_compute<context_t,sender_t>::type
                    >::type ...
                >;

                return prev.template connect<context_t>().push_back(
                    std::apply(
                        [](auto&& ...args){
                            return starter<result_t, __typ__(args)...>(__fwd__(args)...);
                        },
                        connect_inner_sender<uint32_t(0), context_t>()
                    )
                );

            }
        };

        struct
        fn2 {
            template<typename prev_t, typename ...inner_sender_t>
            constexpr
            decltype(auto)
            operator ()(prev_t&& p, inner_sender_t&& ...i) const {
                return _when_all::sender<prev_t, inner_sender_t...>{__fwd__(p), __fwd__(i)...};
            }
        };

        struct
        fn1 {
            template<typename ...sender_t>
            constexpr
            decltype(auto)
            operator ()(sender_t&& ...senders) const {
                __all_must_rval__(senders);
                static_assert(sizeof...(senders) > 0, "no senders");
                return bind_back<fn2, sender_t...>(fn2{}, __fwd__(senders)...);
                //return _when_all::sender(__fwd__(senders)...);
            }
        };
    }

    namespace pusher {
        template<uint32_t pos, typename scope_t>
        requires (pos < std::tuple_size_v<typename scope_t::element_type::op_tuple_type>)
        && (get_current_op_type_t<pos, scope_t>::when_all_reducer)
        struct
        op_pusher<pos, scope_t> : op_pusher_base<pos, scope_t>  {
            template<typename context_t, typename ...value_t>
            static inline
            void
            push_value(context_t& context, scope_t& scope, value_t&& ...v) {
                std::get<pos>(scope->get_op_tuple()).set_value(__fwd__(v)...);
            }

            template<typename context_t>
            static inline
            void
            push_exception(context_t& context, scope_t& scope, std::exception_ptr e) {
                std::get<pos>(scope->get_op_tuple()).set_error(e);
            }

            template<typename context_t>
            static inline
            void
            push_skip(context_t& context, scope_t& scope) {
                std::get<pos>(scope->get_op_tuple()).skip();
            }

            template<typename context_t>
            static inline
            void
            push_done(context_t& context, scope_t& scope) {
                std::get<pos>(scope->get_op_tuple()).done();
            }
        };

        template<uint32_t pos, typename scope_t>
        requires (pos < std::tuple_size_v<typename scope_t::element_type::op_tuple_type>)
        && (get_current_op_type_t<pos, scope_t>::when_all_starter)
        struct
        op_pusher<pos, scope_t> : op_pusher_base<pos, scope_t>  {
            template<typename context_t, typename ...value_t>
            static inline
            void
            push_value(context_t& context, scope_t& scope, value_t&& ...v) {
                std::get<pos>(scope->get_op_tuple())
                    .start(context, scope,
                           _when_all::parent_pusher_status<pos + 1, context_t, scope_t>{context, scope});
            }
        };
    }

    namespace typed {
        template<typename context_t, typename prev_t, typename ...sender_t>
        struct
        compute_sender_type<context_t, pump::_when_all::sender<prev_t, sender_t...>> {
            constexpr static bool multi_values= true;
            using value_type = std::tuple<
                typename pump::when_all_res<
                    typename when_all_compute<context_t,sender_t>::type
                >::type
                ...
            >;
        };
    }

    inline constexpr _when_all::fn1 when_all{};
}
#endif //PUMP_WHEN_ALL_HH
