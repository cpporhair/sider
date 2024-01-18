
#ifndef SIDER_NET_SESSION_SESSION_HH
#define SIDER_NET_SESSION_SESSION_HH

#include "spdk/memory.h"

#include "3rd/ringbuf/ring_buf.hh"

namespace sider::net::session {

    const uint32_t cmd_type_unk     = 0;
    const uint32_t cmd_type_put     = 1;
    const uint32_t cmd_type_get     = 2;

    struct
    cmd {
        uint32_t size{0};
        void *data{nullptr};

        void
        reset() {
            size = 0;
            data = nullptr;
        }

        cmd() = default;
    };

    struct
    unk_cmd {
        uint32_t size{0};
        uint32_t type;
        char data[];
    };

    struct
    put_cmd {
        uint32_t size{0};
        uint32_t type;
        char data[];
    };

    struct
    put_res {

    };

    struct
    get_cmd {
        uint32_t size{0};
        uint32_t type;
        char data[];
    };

    struct
    get_res {

    };

    struct
    session {
        int socket;
        ringbuf_t* cached_data;

        std::list<cmd> unhandled_cmd;
        cmd pending_cmd{};

        explicit
        session(int fd)
            : socket(fd) {
        }

        [[nodiscard]]
        bool
        has_full_command() const {
            auto *p = std::find(cached_data->head, cached_data->tail, '\n');
            return p != nullptr;
        }

        bool
        is_lived() {
            return true;
        }
    };

    auto
    make_session(int socket) {
        return session(socket);
    }
}

#endif //SIDER_NET_SESSION_SESSION_HH
