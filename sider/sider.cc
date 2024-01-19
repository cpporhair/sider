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
execute() {
    return flat_map([](auto &&a) { return handle_command(__fwd__(a)); });
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
