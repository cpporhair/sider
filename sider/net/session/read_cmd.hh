#ifndef SIDER_NET_SESSION_READ_CMD_HH
#define SIDER_NET_SESSION_READ_CMD_HH

#include "sider/pump/then.hh"
#include "sider/pump/flat.hh"
#include "sider/pump/get_context.hh"
#include "sider/net/io_uring/recv.hh"
#include "./session.hh"

namespace sider::net::session {

    inline
    auto
    read_cmd_len() {
        return pump::ignore_args()
            >> pump::get_context<session>()
            >> pump::then([](session &s) {
                return io_uring::recv(s.socket, &(s.pending_cmd.size), 4)
                    >> pump::then([s](...) mutable {
                        s.pending_cmd.data = (void *) (new char[s.pending_cmd.size]);
                    });
            })
            >> pump::flat();
    }

    inline
    auto
    read_cmd_data() {
        return pump::ignore_args()
            >> pump::get_context<session>()
            >> pump::then([](session &s) mutable {
                return sider::net::io_uring::recv(s.socket, s.pending_cmd.data, 4)
                    >> pump::then([s](...) mutable {
                        s.unhandled_cmd.push_back(__mov__(s.pending_cmd));
                        s.pending_cmd.reset();
                    });
            })
            >> pump::flat();
    }

    inline
    auto
    take_first_cmd() {
        return pump::get_context<session>()
            >> pump::then([](session &s) {
                decltype(auto) cmd = __mov__(s.unhandled_cmd.front());
                s.unhandled_cmd.pop_front();
                return cmd;
            });
    }

    inline
    auto
    read_cmd() {
        return read_cmd_len() >> read_cmd_data() >> take_first_cmd();
    }

    inline
    auto
    select_cmd_impl(cmd &&c) {
        using res = std::variant<put_cmd *, get_cmd *>;
        auto *u = (unk_cmd *) &c;
        switch (u->type) {
            case cmd_type_get:
                return res((get_cmd *) u);
            case cmd_type_put:
                return res((put_cmd *) u);
            default:
                throw std::logic_error("unk_cmd");
        }
    }

    inline
    auto
    pick_cmd() {
        return pump::then(select_cmd_impl) >> pump::visit();
    }
}

#endif //SIDER_NET_SESSION_READ_CMD_HH
