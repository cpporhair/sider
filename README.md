# sider

The goal of sider is to implement a redis-like kv store. Use nvme ssd to scale to larger capacities while maintaining speed!

sider uses its own std::execution(p2300) implementation (pump) as an asynchronous and concurrent framework. 
pump isn't perfect, but it's better suited to my project. 

If you want to learn more about p2300, see [here](https://github.com/brycelelbach/wg21_p2300_execution); 

if you want to find a more general, standard, and elegant implementation of std::execution, see [here](https://github.com/NVIDIA/stdexec).

**sider is still in development, not yet tested or used.**

- [x] basic std::execution framework
- [x] basic kv functions
- [x] batch
- [ ] io_uring-based network
- [ ] resp
- [ ] redis datatype

```c++
using namespace sider::coro;
using namespace sider::pump;
using namespace sider::kv;
using namespace ycsb;

uint64_t max_key = 10000000;

int
main(int argc, char **argv){
    start_db(argc, argv)([](){
        return with_context(statistic_helper(new statistic_data()), logger())(
                start_statistic()
                    >> generate_on(any_task_scheduler(), std::views::iota(uint64_t(0), max_key))
                    >> output_statistics_per_sec()
                    >> as_task()
                    >> concurrent(10000)
                    >> as_batch(make_kv() >> put() >> apply() >> statistic_put()) >> statistic_publish()
                    >> count()
                    >> stop_statistic()
                    >> output_finally_statistics()
            )
            >> stop_db();
    });
    return 0;
}
```

Looking for C++/database jobs(china beijing or remote)
