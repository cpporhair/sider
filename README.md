# sider

The goal of sider is to implement a redis-like kv store. Use nvme ssd to scale to larger capacities while maintaining speed!

```c++
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
```


sider has developed its own framework called "[pump](https://github.com/cpporhair/sider/tree/main/sider/pump)" to handle asynchronous and concurrent logic. This is an implementation of c++ std::execution(p2300).
- For learn more about p2300, see [here](https://github.com/brycelelbach/wg21_p2300_execution); 
- For a standard implementation of the p2300, see  [here](https://github.com/NVIDIA/stdexec).

sider uses spdk to implement kernel bypass technology, taking full advantage of nvme's concurrency to speed up io.
- For learn more about spdk, see [here](https://spdk.io);
- In the file [apply.hh](https://github.com/cpporhair/sider/blob/main/sider/kv/apply.hh#L139) you can see how to combine asynchronous and concurrent tasks using the operators provided by pump.

#### **sider is still in development, not yet tested or used.**

- [x] basic std::execution framework
- [x] spdk-based io
- [x] basic kv functions (get,put,scan)
- [x] ycsb test and iops reaches hardware limit.
- [x] batch
- [ ] io_uring-based network
- [ ] cuda support
- [ ] cold and hot data separation


Looking for job opportunities in c++ or database development (beijing or remote)
