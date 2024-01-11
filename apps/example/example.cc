//
//
//

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
#include "sider/kv/stop_db.hh"
#include "sider/net/io_uring/wait_connection.hh"
#include "sider/net/io_uring/recv.hh"
#include "sider/net/io_uring/session.hh"


using namespace sider::coro;
using namespace sider::pump;
using namespace sider::meta;
using namespace sider::net;
using namespace sider::kv;

using session = sider::net::io_uring::session;

inline
auto
read_cmd_len() {
    return ignore_args()
        >> get_context<session>()
        >> then([](session &s) {
            return sider::net::io_uring::recv(s.socket, nullptr, 4);
        })
        >> flat();
}

inline
auto
read_cmd() {
    return ignore_args()
        >> get_context<session>()
        >> then([](session &s) mutable {
            return sider::net::io_uring::recv(s.socket, nullptr, 4);
        })
        >> flat();
}

inline
auto
handle_command() {
    return ignore_args();
}

inline
auto
send_result() {
    return ignore_args();
}

auto
start_session(int fd) {
    return just()
        >> sider::task::schedule_at_any_task()
        >> forever()
        >> read_cmd_len()
        >> read_cmd()
        >> concurrent()
        >> handle_command()
        >> reduce()
        >> submit(make_root_context(session{fd}));
}

inline
auto
wait_connection() {
    return sider::net::io_uring::wait_connection();
}

inline
auto
server_loop() {
    return forever()
        >> ignore_args()
        >> then(wait_connection)
        >> flat()
        >> then(start_session)
        >> reduce();
}

int
main(int argc, char **argv) {
    start_db(argc, argv)([](){
        return forever()
            >> ignore_args()
            >> flat_map(sider::net::io_uring::wait_connection)
            >> concurrent()
            >> flat_map([](int socket){
                return sider::task::just_task()
                    >> with_context(session{socket})([](){
                        return forever()
                            >> ignore_inner_exception(
                                read_cmd()
                                    >> concurrent() >> handle_command()
                                    >> sequential() >> send_result()
                            )
                            >> reduce();
                    });
            })
            >> reduce();
    });
    return 0;
}