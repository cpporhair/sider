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


using namespace sider::coro;
using namespace sider::pump;
using namespace sider::meta;
using namespace sider::task;
using namespace sider::net::io_uring;
using namespace sider::net::session;
using namespace sider::kv;

constexpr uint32_t max_concurrency_per_session = 100;

inline
auto
to_kv(put_cmd* c) {
    return data::key_value(nullptr);
}

inline
auto
to_ke(get_cmd* c) {
    return std::string("");
}

inline
auto
to_result(put_cmd *c) {
    return ignore_results();
}

inline
auto
to_result(get_cmd *c) {
    return ignore_results();
}

inline
auto
handle_command(put_cmd *c) {
    return just()
        >> as_batch(put(to_kv(c)) >> apply() >> ignore_results())
        >> to_result(c);
}

inline
auto
handle_command(get_cmd *c) {
    return just() >> as_batch(get(to_ke(c)) >> ignore_results()) >> to_result(c);
}

inline
auto
handle_command(auto *c) {
    static_assert(false);
}

inline
auto
handle() {
    return flat_map([](auto &&a) { return handle_command(__fwd__(a)); });
}

inline
auto
send_res() {
    return ignore_args();
}

auto
until_session_closed_coro(session& s) -> empty_yields {
    while (s.is_lived())
        co_yield {};
    co_return {};
}

inline
auto
until_session_closed() {
    return get_context<session>()
        >> then([](session &s) { return make_view_able(until_session_closed_coro(s)); })
        >> for_each()
        >> ignore_results();
}

inline
auto
until_session_closed(session &&s){
    return [s = __fwd__(s)](auto&& b) mutable {
        return with_context(__fwd__(s))(
            until_session_closed()
                >> __fwd__(b)
                >> reduce()
        );
    };
};

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
                            >> concurrent() >> pick_cmd() >> handle()
                            >> sequential() >> send_res()
                    );
            })
            >> reduce();
    });
    return 0;
}
