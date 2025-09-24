//

//

#ifndef SIDER_KV_NVME_PUTTER_HH
#define SIDER_KV_NVME_PUTTER_HH

namespace sider::kv::nvme {

    namespace _put_span {
        struct
        req {
            uint64_t sgl_pos_by_page;
            data::write_span& span;
            std::move_only_function<void(data::write_span&)> cb;

            template<typename func_t>
            req(data::write_span& s, func_t&& f)
                : span(s)
                , cb(__fwd__(f))
                , sgl_pos_by_page(0){
            }
        };
        template <typename scheduler_t>
        struct
        op {
            constexpr static bool nvme_put_span_op = true;
            scheduler_t* scheduler;
            data::write_span& span;

            op(scheduler_t* s, data::write_span& p)
                : scheduler(s)
                , span(p){
            }

            op(op&& rhs)
                : scheduler(rhs.scheduler)
                , span(rhs.span){
            }

            template<uint32_t pos, typename context_t, typename scope_t>
            auto
            start(context_t &context, scope_t &scope) {
                return scheduler->schedule(
                    new req (
                        span,
                        [context = context, scope = scope](data::write_span& s) mutable {
                            pump::pusher::op_pusher<pos + 1, scope_t>::push_value(
                                context,
                                scope,
                                s
                            );
                        }
                    )
                );
            }
        };

        template <typename scheduler_t>
        struct
        sender {
            scheduler_t* scheduler;
            data::write_span& span;

            sender(scheduler_t* s, data::write_span& p)
                : scheduler(s)
                , span(p){
            }

            sender(sender&& rhs)
                : scheduler(rhs.scheduler)
                , span(rhs.span){
            }

            inline
            auto
            make_op(){
                return op<scheduler_t> (scheduler, span);
            }

            template<typename context_t>
            auto
            connect() {
                return pump::builder::op_list_builder<0>().push_back(make_op());
            }
        };
    }
}

namespace pump::pusher {
    template<uint32_t pos, typename scope_t>
    requires (pos < std::tuple_size_v<typename scope_t::element_type::op_tuple_type>)
    && (get_current_op_type_t<pos, scope_t>::nvme_put_span_op)
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
    compute_sender_type<context_t, sider::kv::nvme::_put_span::sender<scheduler_t>> {
        using value_type = sider::kv::data::write_span&;
        constexpr static bool multi_values= false;
    };
}

#endif //SIDER_KV_NVME_PUTTER_HH
