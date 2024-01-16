
#ifndef SIDER_NET_RESP_SESSION_HH
#define SIDER_NET_RESP_SESSION_HH

#include "spdk/memory.h"

#include "3rd/ringbuf/ring_buf.hh"

namespace sider::net::resp {

    const uint32_t cmd_type_unk     = 0;
    const uint32_t cmd_type_put     = 1;
    const uint32_t cmd_type_get     = 2;
    const uint32_t cmd_type_scan    = 3;

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
    get_cmd {
        uint32_t size{0};
        uint32_t type;
        char data[];
    };

    struct
    scan_cmd {
        uint32_t size{0};
        uint32_t type;
        char data[];
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

        bool
        has_full_command() {
            return false;
        }
    };
}

#endif //SIDER_NET_RESP_SESSION_HH
