# sider

sider 的目标是建立一个类似 redis 的 kv 存储，使用 nvme ssd 进行扩展，同时保持类似的速度。

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


sider 开发了[pump](https://github.com/cpporhair/sider/tree/main/sider/pump)作为异步和并发的框架。这是一个 c++ std::execution(p2300) 的个人实现。
- 了解更多p2300的信息,点击 [这里](https://github.com/brycelelbach/wg21_p2300_execution); 
- C++标准组有一个标准的P2300实现.正在开发  [这里](https://github.com/NVIDIA/stdexec).

sider使用了spdk实现内核旁路.依靠pump的能力,实现了全链路异步.
- 了解更多spdk的信息, 点击 [这里](https://spdk.io);
- 在函数 [apply](https://github.com/cpporhair/sider/blob/main/sider/kv/apply.hh#L139) 中,你可以看到通过pump是如何组织全链路异步和并发逻辑的

#### **sider是个帮我找工作的展示性项目.目前业余时间开发.无法用于商业**

- [x] basic std::execution framework
- [x] spdk-based io
- [x] basic kv functions (get,put,scan)
- [x] ycsb test and iops reaches hardware limit.
- [x] batch
- [ ] io_uring-based network
- [ ] cuda support
- [ ] cold and hot data separation


下边是我的简要介绍
- 多年数据库内核开发经验,主要负责分布式执行器和KV存储.
- 优点是擅长C++(语言本身以及性能优化).
- 缺点是年龄大,学历低.
- 希望可以找到一份数据库内核或者量化方面的工作.
