//
//
//

#ifndef SIDER_OP_TUPLE_BUILDER_HH
#define SIDER_OP_TUPLE_BUILDER_HH

#include "./scope.hh"

namespace sider::pump {
    template <uint32_t op_pos, typename op_t>
    struct reduce_carrier : public op_t {
        using op_type = op_t;
        constexpr static uint32_t pos = op_pos;
        reduce_carrier(op_t&& op) : op_t(__fwd__(op)) { __must_rval__(op); }
    };

    namespace builder {
        template <typename sender_t>
        struct
        compute_push_context_sender_pos {
            constexpr static uint32_t value = compute_push_context_sender_pos<typename sender_t::prev_type>::value;
        };

        template <typename sender_t>
        requires sender_t::start_sender
        struct
        compute_push_context_sender_pos<sender_t> {
            constexpr static uint32_t value = 0;
        };

        template <typename sender_t>
        requires sender_t::push_context_sender
        struct
        compute_push_context_sender_pos<sender_t> {
            constexpr static uint32_t value = compute_push_context_sender_pos<typename sender_t::prev_type>::value + 1;
        };

        template <typename sender_t>
        constexpr uint32_t compute_push_context_sender_pos_v = compute_push_context_sender_pos<sender_t>::value;

        //////////////////////////////////

        template <uint32_t pos, typename sender_t>
        struct
        compute_pop_context_sender_pos {
            constexpr static uint32_t value = compute_pop_context_sender_pos<pos, typename sender_t::prev_type>::value;
        };

        template <uint32_t pos, typename sender_t>
        requires (sender_t::push_context_sender) && (pos == 0)
        struct
        compute_pop_context_sender_pos <pos, sender_t> {
            constexpr static uint32_t value = sender_t::push_pos;
        };

        template <uint32_t pos, typename sender_t>
        requires (sender_t::push_context_sender) && (pos != 0)
        struct
        compute_pop_context_sender_pos <pos, sender_t> {
            constexpr static uint32_t value = compute_pop_context_sender_pos<pos - 1, typename sender_t::prev_type>::value;
        };

        template <uint32_t pos, typename sender_t>
        requires sender_t::pop_context_sender
        struct
        compute_pop_context_sender_pos <pos, sender_t> {
            constexpr static uint32_t value = compute_pop_context_sender_pos<pos + 1, typename sender_t::prev_type>::value;
        };

        template <uint32_t pos, typename sender_t>
        constexpr uint32_t compute_pop_context_sender_pos_v = compute_pop_context_sender_pos<pos, sender_t>::value;



        template <uint32_t pos, typename ...op_t>
        struct
        __ncp__(op_list_builder) {

            constexpr static uint32_t cur_pos = pos;

            std::tuple<op_t...> ops;

            op_list_builder(std::tuple<op_t...>&& t)
            : ops(__fwd__(t)) {
                __must_rval__(t);
            }
            op_list_builder(){
            }

            op_list_builder(op_list_builder&& rhs)
            : ops(__fwd__(rhs.ops)) {
            }

            template<typename pushed_op_t>
            auto
            push_back(pushed_op_t&& op) {
                return op_list_builder<pos + 1, op_t..., pushed_op_t>(
                    std::tuple_cat(
                        __mov__(ops),
                        std::make_tuple(__fwd__(op))
                    )
                );
            }

            auto
            take(){
                return __mov__(ops);
            }

            constexpr static uint32_t
            get_pos() {
                return pos;
            }
        };

        template <uint32_t pos, typename parent_builder_t, typename stream_builder_t, typename stream_op_maker_t>
        struct
        __ncp__(stream_starter_builder) {
            constexpr static uint32_t cur_pos = pos;

            parent_builder_t parent_builder;
            stream_builder_t stream_builder;
            stream_op_maker_t op_maker;

            stream_starter_builder(parent_builder_t&& p, stream_builder_t&& s, stream_op_maker_t&& o)
                : parent_builder(__fwd__(p))
                , stream_builder(__fwd__(s))
                , op_maker(__fwd__(o)){
                __must_rval__(p);
                __must_rval__(s);
                __must_rval__(o);
            }

            stream_starter_builder(stream_starter_builder&& rhs)
                : parent_builder(__fwd__(rhs.parent_builder))
                , stream_builder(__fwd__(rhs.stream_builder))
                , op_maker(__fwd__(rhs.op_maker)){
            }

            template<typename pushed_op_t>
            auto
            push_back(pushed_op_t&& op) {
                return
                    stream_starter_builder
                        <
                            pos + 1,
                            __typ__(parent_builder),
                            __typ__(stream_builder.push_back(__fwd__(op))),
                            stream_op_maker_t
                        >
                        (
                            __mov__(parent_builder),
                            stream_builder.push_back(__fwd__(op)),
                            __mov__(op_maker)
                        );
            }
            template<typename ctrl_op_t>
            auto
            push_ctrl_op(ctrl_op_t&& op) {
                return push_back(reduce_carrier<pos, ctrl_op_t>(__fwd__(op)));
            }

            auto
            to_op() {
                return op_maker.make_op(stream_builder.take());
            }

            template<typename reduce_op_t>
            auto
            push_reduce_op(reduce_op_t&& op) {
                return parent_builder.push_back(push_back(__fwd__(op)).to_op());
            }

            constexpr static uint32_t
            get_reduce_op_next_pos() {
                return parent_builder_t::cur_pos + 1;
            }

            constexpr static uint32_t
            get_con_or_seq_op_next_pos() {
                return cur_pos + 1;
            }
        };
    }

    template <typename op_tuple_t, typename new_op_t>
    inline
    auto
    push_front_to_op_list(op_tuple_t&& t, new_op_t&& n) {
        __must_rval__(t);
        __must_rval__(n);
        return std::tuple_cat(std::make_tuple<new_op_t>(__fwd__(n)), __fwd__(t));
    }

    template <typename op_tuple_t, typename new_op_t>
    inline
    auto
    push_back_to_op_list(op_tuple_t&& t, new_op_t&& n) {
        return std::tuple_cat(__fwd__(t), std::make_tuple(__fwd__(n)));
    }

    template <typename op_tuple_t, typename new_op_t>
    inline
    auto
    push_back_to_op_list(op_tuple_t&& t, new_op_t& n) {
        return std::tuple_cat(__fwd__(t), std::tie(n));
    }
}
#endif //SIDER_OP_TUPLE_BUILDER_HH
