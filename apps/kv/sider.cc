#include <exception>
#include "util/macro.hh"
#include "pump/flat.hh"
#include "pump/repeat.hh"
#include "pump/sequential.hh"
#include "pump/reduce.hh"
#include "pump/visit.hh"
#include "pump/just.hh"
#include "pump/when_all.hh"

#include "sider/kv/start_db.hh"
#include "sider/kv/put.hh"
#include "sider/kv/get.hh"
#include "sider/kv/apply.hh"
#include "sider/kv/start_batch.hh"
#include "sider/kv/finish_batch.hh"
#include "sider/kv/stop_db.hh"
#include "sider/net/io_uring/wait_connection.hh"
#include "sider/net/io_uring/recv.hh"
#include "sider/net/session/read_cmd.hh"
#include "sider/net/session/until_session_closed.hh"
#include "sider/net/session/send_result.hh"

#include "./proto/command.pb.h"


using namespace sider::coro;
using namespace sider::pump;
using namespace sider::meta;
using namespace sider::task;
using namespace sider::net::io_uring;
using namespace sider::net::session;
using namespace sider::kv;

constexpr uint32_t max_concurrency_per_session = 100;

template <uint32_t tag_v>
struct
msg_with_tag {
    msg_request req;
    msg_request res;
};

inline
auto
to_kv_impl(const msg_put_req& c) {
    return data::key_value(nullptr);
}

inline
auto
to_kv(msg_with_tag<msg_type::put_req>& c) {
    return to_kv_impl(c.req.put_req());
}

inline
auto
to_key(msg_with_tag<msg_type::get_req>& c) {
    return std::string("");
}

inline
auto
to_result(msg_request& res) {
    return ignore_results();
}

inline
auto
handle_command(msg_with_tag<msg_type::put_req>&& c) {
    return just()
        >> with_context(__fwd__(c))([]() {
            return as_batch()([]() {
                return get_context<msg_with_tag<msg_type::put_req>>()
                    >> then(to_kv)
                    >> put()
                    >> apply()
                    >> ignore_args();
            });
        })
        >> to_result(c.res);
}

inline
auto
handle_command(msg_with_tag<msg_type::get_req>&& c) {
    return just()
        >> with_context(__fwd__(c))([]() {
            return as_batch()([]() {
                return get_context<msg_with_tag<msg_type::get_req>>()
                    >> then(to_key)
                    >> get()
                    >> ignore_results();
            });
        })
        >> to_result(c.res);
}

inline
auto
handle_command(auto&& c) {
    static_assert(false);
}

inline
auto
execute() {
    return flat_map([](auto &&a) { return handle_command(__fwd__(a)); });
}

inline
auto
select_cmd_impl(cmd &&c) {
    using res = std::variant<msg_with_tag<msg_type::put_req>, msg_with_tag<msg_type::get_req>>;
    msg_request msg;
    msg.ParseFromArray((void *) c.data, c.size);
    switch (msg.type()) {
        case msg_type::put_req:
            return res(msg_with_tag<msg_type::put_req>{__mov__(msg), {}});
        case msg_type::get_req:
            return res(msg_with_tag<msg_type::get_req>{__mov__(msg), {}});
        default:
            throw std::logic_error("unk_cmd");
    }
}

inline
auto
pick_cmd() {
    return then(select_cmd_impl) >> visit();
}

int
main(int argc, char **argv) {
    start_db(argc, argv)([](){
        return forever()
            >> flat_map(wait_connection)
            >> concurrent()
            >> flat_map([](int fd){
                return start_on(random_core())
                    >> until_session_closed(make_session(fd))(
                        read_cmd()
                            >> concurrent() >> pick_cmd() >> execute()
                            >> sequential() >> send_res()
                    );
            })
            >> reduce();
    });
    return 0;
}
