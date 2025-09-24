#ifndef SIDER_NET_IO_URING_ACCEPT_SCHEDULER_HH
#define SIDER_NET_IO_URING_ACCEPT_SCHEDULER_HH

#include <liburing.h>
#include <list>
#include "spdk/env.h"

#include "sider/pump/then.hh"
#include "./io_uring_request.hh"

namespace sider::net::io_uring {
    namespace _wait_connection {

        struct
        req {
            std::move_only_function<void(int)> cb;
        };

        template <typename scheduler_t>
        struct
        op {
            constexpr static bool net_wait_connection_op = true;
            scheduler_t* scheduler;

            op(scheduler_t* s)
                : scheduler(s) {
            }

            op(op&& rhs)
                : scheduler(rhs.scheduler) {
            }

            template<uint32_t pos, typename context_t, typename scope_t>
            auto
            start(context_t &context, scope_t &scope) {
                return scheduler->schedule(
                    new req{
                        [context = context, scope = scope](int client_fd) mutable {
                            pump::pusher::op_pusher<pos + 1, scope_t>::push_value(
                                context,
                                scope,
                                client_fd
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
    accept_scheduler {
        friend struct _wait_connection::op<accept_scheduler>;
    private:
        struct io_uring ring{};
        std::list<int> client_socks;
        uint32_t base_core;
        spdk_ring* wat_connection_queue;
        int server_socket;
    private:
        auto
        schedule(_wait_connection::req* req) {
            assert(0 < spdk_ring_enqueue(wat_connection_queue, (void**)&req, 1, nullptr));
        }

        auto
        add_accept_request(int socket, struct sockaddr_in *client_addr,
                               socklen_t *client_addr_len) {
            struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
            io_uring_prep_accept(sqe, socket, (struct sockaddr *) client_addr,
                                 client_addr_len, 0);
            io_uring_request *req = (io_uring_request*)malloc(sizeof(*req));
            req->event_type = uring_event_type::accept;
            io_uring_sqe_set_data(sqe, req);
            io_uring_submit(&ring);

            return 0;
        }

        void
        handle_wait_connection() {
            while(!client_socks.empty()) {
                _wait_connection::req *req = nullptr;
                if (0 == spdk_ring_dequeue(wat_connection_queue, (void **) req, 1))
                    break;
                int fd = client_socks.front();
                client_socks.pop_front();
                req->cb(fd);
                delete req;
            }

            sockaddr_in client_addr{};
            socklen_t client_addr_len = sizeof(client_addr);
            io_uring_cqe *cqe;
            add_accept_request(server_socket, &client_addr, &client_addr_len);
            while (io_uring_peek_cqe(&ring, &cqe) == 0) {
                if (cqe->res < 0)[[unlikely]]
                    break;
                auto *uring_req = (io_uring_request *) cqe->user_data;
                if (uring_req->event_type == uring_event_type::accept) {
                    add_accept_request(server_socket, &client_addr, &client_addr_len);
                    _wait_connection::req *waiting_req = nullptr;
                    if (1 == spdk_ring_dequeue(wat_connection_queue, (void **) waiting_req, 1)){
                        waiting_req->cb((int) cqe->res);
                        delete waiting_req;
                        free(uring_req);
                    }
                    else {
                        client_socks.push_back((int) cqe->res);
                        free(uring_req);
                    }
                }

                io_uring_cqe_seen(&ring, cqe);
            }
        }
    public:
        explicit
        accept_scheduler(uint32_t core)
            : wat_connection_queue(spdk_ring_create(spdk_ring_type::SPDK_RING_TYPE_MP_SC, 10, 0))
            , base_core(core)
            , server_socket(0){

        }

        void
        listen_at(uint32_t port) {
            io_uring_queue_init(256, &ring, 0);
        }

        auto
        wait_connection() {
            return _wait_connection::sender<accept_scheduler>(this);
        }

        void
        advance() {
            handle_wait_connection();
        }
    };
}

namespace pump::pusher {
    template<uint32_t pos, typename scope_t>
    requires (pos < std::tuple_size_v<typename scope_t::element_type::op_tuple_type>)
    && (get_current_op_type_t<pos, scope_t>::net_wait_connection_op)
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
    compute_sender_type<context_t, sider::net::io_uring::_wait_connection::sender<scheduler_t>> {
        constexpr static bool has_value_type = true;
        using value_type = int;
        constexpr static bool multi_values = false;
    };
}

#endif //SIDER_NET_IO_URING_ACCEPT_SCHEDULER_HH
