//
// Created by null on 24-1-11.
//

#ifndef SIDER_NET_IO_URING_SESSION_SCHEDULER_HH
#define SIDER_NET_IO_URING_SESSION_SCHEDULER_HH

#include <functional>
#include "liburing.h"

#include "sider/pump/op_pusher.hh"

namespace sider::net::io_uring {
    namespace _recv {
        struct
        req {
            int socket;
            void* buf;
            uint32_t size;
            std::move_only_function<void(int, void *, uint32_t, bool)> cb;
        };

        template <typename scheduler_t>
        struct
        op {
            constexpr static bool net_io_uring_recv_op = true;
            scheduler_t* scheduler;
            int socket;
            void* buffer;
            uint32_t size;

            op(scheduler_t* s, int sock, void* buf, uint32_t len)
                : scheduler(s)
                , socket(sock)
                , buffer(buf)
                , size(len){
            }

            op(op&& rhs)
                : scheduler(rhs.scheduler) {
            }

            template<uint32_t pos, typename context_t, typename scope_t>
            auto
            start(context_t &context, scope_t &scope) {
                return scheduler->schedule(
                    new req{
                        socket,
                        buffer,
                        size,
                        [context = context, scope = scope](int sock, void * buf, uint32_t len, bool status) mutable {
                            pump::pusher::op_pusher<pos + 1, scope_t>::push_value(
                                context,
                                scope,
                                sock,
                                buf,
                                len,
                                status
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
            int socket;
            void* buffer;
            uint32_t size;

            sender(scheduler_t* s, int sock, void* buf, uint32_t len)
                : scheduler(s) {
            }

            sender(sender &&rhs)
                : scheduler(rhs.scheduler) {
            }

            inline
            auto
            make_op(){
                return op<scheduler_t>(scheduler, socket, buffer, size);
            }

            template<typename context_t>
            auto
            connect() {
                return pump::builder::op_list_builder<0>().push_back(make_op());
            }
        };
    }

    struct
    session_scheduler {
        friend struct _recv::op<session_scheduler>;
    private:
        struct io_uring ring{};
    private:
        auto
        schedule(_recv::req* req) {
            io_uring_sqe *sqe = io_uring_get_sqe(&ring);
            auto* io_req = (io_uring_request*)malloc(sizeof(io_uring_request) + sizeof(struct iovec));
            io_req->iov[0].iov_base = req->buf;
            io_req->iov[0].iov_len = req->size;
            io_req->event_type = uring_event_type::read;
            io_req->client_socket = req->socket;
            io_req->user_data = req;
            io_uring_prep_readv(sqe, req->socket, &io_req->iov[0], 1, 0);
            io_uring_sqe_set_data(sqe, req);
        }

        void
        handle_io() {
            io_uring_cqe *cqe;
            while (io_uring_peek_cqe(&ring, &cqe) == 0) {
                if (cqe->res < 0)[[unlikely]]
                    break;
                auto *uring_req = (io_uring_request *) cqe->user_data;
                switch (uring_req->event_type) {
                    case uring_event_type::read: {
                        auto *req = (_recv::req*)uring_req->user_data;
                        req->cb(req->socket, req->buf, req->size, true);
                        delete req;
                        free(uring_req);
                        break;
                    }
                    case uring_event_type::write: {
                        auto *req = (_recv::req*)uring_req->user_data;
                        req->cb(req->socket, req->buf, req->size, true);
                        delete req;
                        free(uring_req);
                        break;
                    }
                    default: {
                        free(uring_req);
                        break;
                    }
                }

                io_uring_cqe_seen(&ring, cqe);
            }
        }
    public:
        auto
        recv(int socket, void* buf, uint32_t size) {
            return _recv::sender<session_scheduler>(this, socket, buf, size);
        }

        void
        advance() {
            handle_io();
        }
    };
}

namespace sider::pump::pusher {
    template<uint32_t pos, typename scope_t>
    requires (pos < std::tuple_size_v<typename scope_t::element_type::op_tuple_type>)
    && (get_current_op_type_t<pos, scope_t>::net_io_uring_recv_op)
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
    compute_sender_type<context_t, sider::net::io_uring::_recv::sender<scheduler_t>> {
        constexpr static bool has_value_type = true;
        using value_type = std::tuple<int, void *, uint32_t, bool>;
        constexpr static bool multi_values = true;
    };
}

#endif //SIDER_NET_IO_URING_SESSION_SCHEDULER_HH
