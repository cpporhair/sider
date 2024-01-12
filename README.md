# sider

The goal of sider is to implement a redis-like kv store. Use nvme ssd to scale to larger capacities while maintaining speed!

sider uses its own std::execution(p2300) implementation (pump) as an asynchronous and concurrent framework.

For learn more about p2300, see [here](https://github.com/brycelelbach/wg21_p2300_execution); 

For a standard implementation of the p2300, see  [here](https://github.com/NVIDIA/stdexec).

**sider is still in development, not yet tested or used.**

- [x] basic std::execution framework
- [x] basic kv functions
- [x] batch
- [ ] io_uring-based network
- [ ] resp
- [ ] redis datatype

```c++
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
```

Looking for job opportunities in c++ or database development (beijing or remote)
