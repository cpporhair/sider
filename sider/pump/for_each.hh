//
//
//

#ifndef SIDER_PUMP_FOR_EACH_HH
#define SIDER_PUMP_FOR_EACH_HH

#include <memory>
#include <ranges>
#include <iostream>

#include "./helper.hh"
#include "./op_pusher.hh"
#include "./op_tuple_builder.hh"
#include "./compute_sender_type.hh"
#include "./bind_back.hh"

namespace sider::pump {
    namespace _for_each_ranges {
        template <uint32_t pos, typename ranges_t>
        struct
        runner {
            constexpr static bool for_each_ranges_runner = true;
            constexpr static uint32_t parent_pusher_pos = pos;
            ranges_t range;
            std::ranges::iterator_t<ranges_t> it;

            runner(ranges_t&& r)
                : range(__fwd__(r))
                , it(range.begin()){
            }

            runner(runner&& o)noexcept
                : range(__fwd__(o.range))
                , it(__mov__(o.it)){
            }

            runner(const runner& o)noexcept
                : range(o.range)
                , it(range.begin()){
            }

            bool
            is_end() {
                return it == range.end();
            }

            inline
            auto&
            poll_one()requires std::is_lvalue_reference_v<decltype(*it)> {
                auto& x = *it;
                it++;
                return x;
            }

            inline
            auto
            poll_one() {
                auto x = *it;
                it++;
                return x;
            }


            void
            reset() {
                it = range.begin();
            }
        };
        template <std::ranges::viewable_range ranges, typename stream_op_tuple_t>
        struct
        starter {
            constexpr static bool for_each_ranges_starter = true;
            ranges range;
            stream_op_tuple_t stream_op_tuple;

            starter(ranges&& r, stream_op_tuple_t&& ops)
                : range(__fwd__(r))
                , stream_op_tuple(__fwd__(ops)){
            }

            starter(starter&& o)noexcept
                : range(__fwd__(o.range))
                , stream_op_tuple(__fwd__(o.stream_op_tuple)){
            }

            starter(const starter& o)noexcept
                : range(o.range)
                , stream_op_tuple(o.stream_op_tuple){
            }

        };

        template <typename range_t>
        struct
        __ncp__(op_maker) {
            range_t range;

            constexpr static bool has_size = std::is_same_v<__typ__(std::size(range)), __typ__(std::size(range))>;

            op_maker(range_t&& r)
                : range(__fwd__(r)){
            }

            op_maker(op_maker&& r)
                : range(__fwd__(r.range)){
            }

            template <typename op_tuple_t>
            inline
            auto
            make_op(op_tuple_t&& ops) {
                __must_rval__(ops);
                if constexpr (std::is_lvalue_reference_v<range_t>)
                    return starter<range_t,op_tuple_t>(__fwd__(range), __fwd__(ops));
                else
                    return starter(__mov__(range), __fwd__(ops));
            }
        };

        template <typename prev_t, std::ranges::viewable_range ranges_t>
        struct
        __ncp__(sender) {

            using prev_type = prev_t;

            ranges_t range;
            prev_t prev;

            explicit
            sender(prev_t&& p, ranges_t&& r)
                : prev(__fwd__(p))
                , range(__fwd__(r)){
            }

            sender(sender&& o)
                : prev(__fwd__(o.prev))
                , range(__fwd__(o.range)){
            }

            template<typename context_t>
            auto
            connect(){
                return builder::stream_starter_builder<
                    1,
                    __typ__(prev.template connect<context_t>()),
                    builder::op_list_builder<0>,
                    op_maker<ranges_t>
                > (
                    prev.template connect<context_t>(),
                    builder::op_list_builder<0>(),
                    op_maker<ranges_t>(__fwd__(range))
                );
            }
        };
    }

    namespace _for_each_byargs {
        template <typename stream_op_tuple_t>
        struct
        starter {
            constexpr static bool for_each_byargs_starter = true;
            stream_op_tuple_t stream_op_tuple;

            starter(stream_op_tuple_t&& ops)
                : stream_op_tuple(__fwd__(ops)){
            }

            starter(starter&& o)noexcept
                : stream_op_tuple(__fwd__(o.stream_op_tuple)){
            }

            starter(const starter& o)noexcept
                : stream_op_tuple(o.stream_op_tuple){
            }

        };
        struct
        op_maker {
            op_maker() {
            }

            template <typename op_tuple_t>
            inline
            auto
            make_op(op_tuple_t&& ops) {
                return starter(__fwd__(ops));
            }
        };

        template <typename prev_t>
        struct
        __ncp__(sender) {

            using prev_type = prev_t;
            prev_t prev;

            explicit
            sender(prev_t&& p)
                : prev(__fwd__(p)) {
            }

            sender(sender&& o)
                : prev(__fwd__(o.prev)) {
            }

            template<typename context_t>
            auto
            connect(){
                return builder::stream_starter_builder<
                    1,
                    __typ__(prev.template connect<context_t>()),
                    builder::op_list_builder<0>,
                    op_maker
                > (
                    prev.template connect<context_t>(),
                    builder::op_list_builder<0>(),
                    op_maker()
                );
            }
        };
    }

    namespace _for_each {
        struct
        fn {
            template <typename sender_t, std::ranges::viewable_range range_t>
            constexpr
            decltype(auto)
            operator ()(sender_t&& s, range_t&& r) const {
                return _for_each_ranges::sender<sender_t, range_t>{__fwd__(s), __fwd__(r)};
            }

            template <std::ranges::viewable_range range_t>
            constexpr
            decltype(auto)
            operator ()(range_t&& r) const {
                return bind_back<fn, range_t>(fn{}, __fwd__(r));
            }

            template <typename sender_t>
            constexpr
            decltype(auto)
            operator ()(sender_t&& s) const {
                return _for_each_byargs::sender<sender_t>{__fwd__(s)};
            }

            template<typename ...any_t>
            constexpr
            decltype(auto)
            operator ()() const {
                return bind_back<fn>(fn{});
            }
        };
    }

    namespace pusher {
        template<uint32_t pos, typename scope_t>
        requires (pos < std::tuple_size_v<typename scope_t::element_type::op_tuple_type>)
        && (get_current_op_type_t<pos, scope_t>::for_each_ranges_runner)
        struct
        op_pusher<pos, scope_t> : op_pusher_base<pos, scope_t> {

            template<typename context_t>
            static inline
            void
            poll_next(context_t& context, scope_t& scope) {
                decltype(auto) x = scope->get_op_tuple();
                auto& op = std::get<pos>(x);
                if (!op.is_end())
                    op_pusher<pos + 1, scope_t>::push_value(context, scope, op.poll_one());
                else
                    op_pusher<pos + 1, scope_t>::push_done(context, scope);
            }

            template<typename context_t>
            static inline
            void
            start(context_t& context, scope_t& scope) {
                auto& op = std::get<pos>(scope->get_op_tuple());
                if (!op.is_end())
                    ::sider::pump::pusher::op_pusher<pos + 1, scope_t>::push_value(context, scope, op.poll_one());
                else
                    ::sider::pump::pusher::op_pusher<pos + 1, scope_t>::push_done(context, scope);
            }
        };

        template<uint32_t pos, typename scope_t>
        requires (pos < std::tuple_size_v<typename scope_t::element_type::op_tuple_type>)
        && (get_current_op_type_t<pos, scope_t>::for_each_ranges_starter)
        struct
        op_pusher<pos, scope_t> : op_pusher_base<pos, scope_t> {
            template<typename context_t, typename stream_scope_t>
            static inline
            void
            start_stream(context_t& context, stream_scope_t&& stream_scope) {
                pusher::op_pusher<0, stream_scope_t>::start(context, stream_scope);
            }

            template<typename context_t>
            static inline
            void
            push_value(context_t& context, scope_t& scope) {
                auto& op = std::get<pos>(scope->get_op_tuple());
                start_stream(
                    context,
                    make_runtime_scope<runtime_scope_type::stream_starter>(
                        scope,
                        std::tuple_cat(
                            std::make_tuple(
                                _for_each_ranges::runner<pos, __typ__(op.range)&>(op.range)
                            ),
                            tuple_to_tie(op.stream_op_tuple)
                        )
                    )
                );
            }
        };

        template<uint32_t pos, typename scope_t>
        requires (pos < std::tuple_size_v<typename scope_t::element_type::op_tuple_type>)
        && (get_current_op_type_t<pos, scope_t>::for_each_byargs_starter)
        struct
        op_pusher<pos, scope_t> : op_pusher_base<pos, scope_t> {
            template<typename context_t, typename stream_scope_t>
            static inline
            void
            start_stream(context_t& context, stream_scope_t&& stream_scope) {
                pusher::op_pusher<0, stream_scope_t>::start(context, stream_scope);
            }

            template<typename context_t, typename value_t>
            static inline
            void
            push_value(context_t& context, scope_t& scope, value_t&& v) {
                static_assert(std::ranges::viewable_range<value_t>);
                auto& op = std::get<pos>(scope->get_op_tuple());
                start_stream(
                    context,
                    make_runtime_scope<runtime_scope_type::stream_starter>(
                        scope,
                        std::tuple_cat(
                            std::make_tuple(
                                _for_each_ranges::runner<pos, value_t>(__mov__(v))
                            ),
                            tuple_to_tie(op.stream_op_tuple)
                        )
                    )
                );
            }

            template<typename context_t, typename value_t>
            static inline
            void
            push_value(context_t& context, scope_t& scope, value_t&& v)
            requires std::is_lvalue_reference_v<value_t> {
                static_assert(std::ranges::viewable_range<value_t>);
                auto& op = std::get<pos>(scope->get_op_tuple());
                start_stream(
                    context,
                    make_runtime_scope<runtime_scope_type::stream_starter>(
                        scope,
                        std::tuple_cat(
                            std::make_tuple(
                                _for_each_ranges::runner<pos, value_t>(__fwd__(v))
                            ),
                            tuple_to_tie(op.stream_op_tuple)
                        )
                    )
                );
            }
        };
    }

    namespace typed {
        template <typename context_t, typename sender_t, std::ranges::viewable_range ranges_t>
        struct
        compute_sender_type<context_t, sider::pump::_for_each_ranges::sender<sender_t, ranges_t>> {
            using value_type = std::ranges::iterator_t<ranges_t>::value_type;
            constexpr static bool has_value_type = true;
        };

        template <typename context_t, typename sender_t>
        struct
        compute_sender_type<context_t, sider::pump::_for_each_byargs::sender<sender_t>> {
            using value_type = std::ranges::iterator_t<typename compute_sender_type<context_t, sender_t>::value_type>::value_type;
            constexpr static bool has_value_type = true;
            //using value_type = std::ranges::iterator_t<std::decay_t<typename compute_sender_type<context_t, sender_t>::value_type>>::value_type;
        };
    }

    inline constexpr _for_each::fn for_each{};

    inline
    auto
    for_each_by_args() {
        return for_each();
    }

    template<typename range_t>
    struct
    range_ref_box {
        range_t& range;

        range_ref_box(range_t& r)
            : range(r) {
        }

        range_ref_box(range_ref_box&& rhs)
            : range(rhs.range) {
        }

        auto
        begin() {
            return range.begin();
        }

        auto
        end() {
            return range.end();
        }

        auto&
        operator
        = (range_ref_box const&) = delete;

        auto&
        operator
        = (range_ref_box&& o) noexcept {
            std::swap(range,o.range);
            return *this;
        }

    };

    template<typename range_t>
    auto
    package_range_ref(range_t& r) {
        return range_ref_box(r);
    }

}
#endif //SIDER_PUMP_FOR_EACH_HH
