//
//
//

#include "liburing.h"

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


using namespace sider::coro;
using namespace sider::pump;
using namespace sider::meta;

int
main(int argc, char **argv) {

    just()
        >> when_all(
            just(1) >> then([](int a) { std::cout << a << std::endl; }),
            just(2) >> then([](int a) { std::cout << a << std::endl; })
        )
        >> ignore_results()
        >> for_each(std::views::iota(1, 10))
        >> then([](int a) { return a % 2 == 0; })
        >> visit()
        >> concurrent()
        >> then([](auto &&a) {
            if constexpr (is_true<__typ__(a)>)
                return just() >> then([a = __fwd__(a)]() { std::cout << "even-numbered" << a << std::endl; });
            else
                return just();
        })
        >> flat()
        >> ignore_results()
        >> reduce()
        >> submit(make_root_context());
    return 0;
}