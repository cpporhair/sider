//

//

#ifndef SIDER_KV_NVME_GETTER_HH
#define SIDER_KV_NVME_GETTER_HH

namespace sider::kv::nvme {
    namespace _get_data {
        struct
        req {
            data::data_page* page;
            std::move_only_function<void(data::data_page*)> cb;

            inline
            auto
            get_payload() {
                return page->payload;
            }

            inline
            auto
            get_page() {
                return page;
            }

            inline
            auto
            get_page_pos() {
                return page->pos;
            }

            inline
            auto
            set_wrote(bool b) {
                return page->set_wrote(b);
            }

            inline
            auto
            get_payload_size(){
                return runtime::data_page_size;
            }
        };
        template <typename scheduler_t>
        struct
        op {
            constexpr static bool nvme_get_data_op = true;
            scheduler_t* scheduler;
            data::data_page* page;

            op(scheduler_t* s, data::data_page* p)
                : scheduler(s)
                , page(p){
            }

            op(op&& rhs)
                : scheduler(rhs.scheduler)
                , page(rhs.page){
            }

            template<uint32_t pos, typename context_t, typename scope_t>
            auto
            start(context_t &context, scope_t &scope) {
                return scheduler->schedule(
                    new req {
                        page,
                        [context = context, scope = scope](data::data_page* p) mutable {
                            pump::pusher::op_pusher<pos + 1, scope_t>::push_value(
                                context,
                                scope,
                                p
                            );
                        }
                    }
                );
            }
        };

        template <typename scheduler_t>
        struct
        sender {
            scheduler_t* scheduler;
            data::data_page* page;

            sender(scheduler_t* s, data::data_page* p)
                : scheduler(s)
                , page(p){
            }

            sender(sender&& rhs)
                : scheduler(rhs.scheduler)
                , page(rhs.page){
            }

            inline
            auto
            make_op(){
                return op<scheduler_t> (scheduler, page);
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
    && (get_current_op_type_t<pos, scope_t>::nvme_get_data_op)
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
    compute_sender_type<context_t, sider::kv::nvme::_get_data::sender<scheduler_t>> {
        using value_type = sider::kv::data::write_span&;
        constexpr static bool multi_values= false;
    };
}

#endif //SIDER_KV_NVME_GETTER_HH
