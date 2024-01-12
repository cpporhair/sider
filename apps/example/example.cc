//
//
//
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
#include "sider/net/resp/read_cmd.hh"


using namespace sider::coro;
using namespace sider::pump;
using namespace sider::meta;
using namespace sider::task;
using namespace sider::net::io_uring;
using namespace sider::net::resp;
using namespace sider::kv;

inline
auto
select_cmd(cmd&& c) {
    using res = std::variant<put_cmd *, get_cmd *, scan_cmd *>;
    auto *u = (unk_cmd *) &c;
    switch (u->type) {
        case cmd_type_get:
            return res((get_cmd *) u);
        case cmd_type_put:
            return res((put_cmd *) u);
        case cmd_type_scan:
            return res((scan_cmd *) u);
        default:
            throw std::logic_error("unk_cmd");
    }
}

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
to_result() {
    return ignore_results();
}

inline
auto
handle_command() {
    return then([](cmd&& c){ return select_cmd(__fwd__(c)); })
        >> visit()
        >> then([](auto&& a) {
            if constexpr (std::is_same_v<std::remove_pointer_t<__typ__(a)>, put_cmd>)
                return just() >> as_batch( put(to_kv(a)) >> apply() >> to_result() );
            else if constexpr (std::is_same_v<std::remove_pointer_t<__typ__(a)>, get_cmd>)
                return just() >> as_batch( get(to_ke(a)) >> to_result() );
            else
                return just();
        })
        >> flat();
}

inline
auto
send_result() {
    return ignore_args();
}

int
main(int argc, char **argv) {
    start_db(argc, argv)([](){
        return forever()
            >> flat_map(wait_connection)
            >> concurrent()
            >> flat_map([](int socket){
                return start_as_task()
                    >> push_context(session{socket})
                    >> forever()
                    >> read_cmd()
                    >> concurrent(100)
                    >> ignore_inner_exception(
                        handle_command() >> send_result()
                    )
                    >> reduce()
                    >> pop_context();
            })
            >> reduce();
    });
    return 0;
}