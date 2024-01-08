//
//
//

#include "util/macro.hh"
#include "util/meta.hh"
#include "pump/flat.hh"
#include "pump/repeat.hh"
#include "pump/sequential.hh"
#include "pump/reduce.hh"
#include "pump/visit.hh"
#include "pump/just.hh"
#include "pump/when_all.hh"
#include "pump/push_context.hh"
#include "pump/pop_context.hh"
#include "pump/generate.hh"
#include "pump/maybe.hh"
#include "sider/task/any.hh"

#include "sider/kv/start_db.hh"
#include "sider/kv/start_batch.hh"
#include "sider/kv/put.hh"
#include "sider/kv/make_kv.hh"
#include "sider/kv/apply.hh"
#include "sider/kv/scan.hh"
#include "sider/kv/stop_db.hh"
#include "sider/kv/get.hh"


using namespace sider::coro;
using namespace sider::pump;
using namespace sider::meta;
using namespace sider::kv;

using name = std::string;

auto
generate_kv_seed(uint32_t count) -> return_yields<uint64_t> {
    for (uint64_t i = 0; i < count; ++i)
        co_yield i;
    co_return 0;
}

auto
new_kv(uint64_t seed) {
    return then([](...){
        return (data::key_value*)nullptr;
    });
}

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