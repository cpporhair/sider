#include <expected>

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


using namespace pump::coro;
using namespace pump::meta;
using namespace pump;

auto
foo_coro() -> co_object<return_promise<std::tuple<int,float,int>>> {
    int n = 0;
    std::cin >> n;
    co_return std::make_tuple(n, 1.1, 1);
}

template <typename tag_t, typename ...data_t>
struct
tag {
    using tag_type = tag_t;
    std::tuple<data_t...> data;
};

template <typename tag_t, typename data_t>
struct
tag<tag_t, data_t> {
    using tag_type = tag_t;
    data_t data;
};

template <typename tag_t, typename ...data_t>
auto
tag_data(data_t&& ...data){
    return tag<tag_t, data_t...>{__fwd__(data)...};
}

struct tag_1 {};
struct tag_2 {};

auto
test_n(auto&& a) {
    if constexpr (is_true<__typ__(a)>)
        return tag_data<tag_1>(just() >> then([a = __fwd__(a)]() { std::cout << "even-numbered" << a << std::endl; }));
    else
        return tag_data<tag_2>(just());
}

struct
test_n {
    template <typename T>
    auto
    operator()(T&& a) const {
        if constexpr (is_true<__typ__(a)>)
            return tag_data<tag_1>(just() >> then([a = __fwd__(a)]() { std::cout << "even-numbered" << a << std::endl; }));
        else
            return tag_data<tag_2>(just());
    }
};

inline constexpr struct test_n tn{};

int
main(int argc, char **argv) {
    auto a = just()
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
        >> reduce();

    __mov__(a) >> submit(make_root_context());

    return 0;
}