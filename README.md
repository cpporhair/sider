# sider

sider 是一个为了找工作而展示的项目,目的是为了更好的说明我目前的技术方向和能力,无法应用于实际项目,请注意.

sider 是我负责的一个叫做monism的项目的非常简要的开源实现,仅仅展示了部分pump框架和极其简略的kv实现
- monism的目标是替代rocksdb作为公司分布式数据库xtp的存储引擎
- 强一致,支持mvcc,读写分离,内核旁路.零copy,无锁.
- 速度是rocksdb的5.7倍,尾部延迟是rocksdb的30%

sider 独立开发了[pump](https://github.com/cpporhair/sider/tree/main/sider/pump)作为异步和并发的框架。这是一个 c++ std::execution(p2300) 的个人实现。
- 了解更多p2300的信息,点击 [这里](https://github.com/brycelelbach/wg21_p2300_execution);
- C++标准组有一个标准的P2300实现.正在开发  [这里](https://github.com/NVIDIA/stdexec).
- pump和标准组的实现略有不同
    - pump扩展了scheduler的概念.不限于硬件环境的调度,也可以应用于纯逻辑(比如索引或者页面分配和释放)的调度
    - pump在build阶段尽量不做类型检查和计算,这样可以支持visit这类的编译期算子.
    - pump把执行过程抽象成一个执行计划树.着重于超高并发时候的效率.并为将来对执行计划进行编译期或者非编译期的优化留下了空间
    - pump对于协程的适配,使用generator模式

sider使用了spdk实现内核旁路.依靠pump的能力,实现了全链路异步.
- 了解更多spdk的信息, 点击 [这里](https://spdk.io);
- 在函数 [apply](https://github.com/cpporhair/sider/blob/main/sider/kv/apply.hh#L139) 中,你可以看到通过pump是如何组织全链路异步和并发逻辑的

```
...
auto
write_data(data::write_span_list& list){
    return pump::for_each(list.spans)                                             / * 对于每一个 span * /
        >> pump::concurrent()                                                     / * 并发执行 * /
        >> pump::then([](data::write_span& span){ return nvme::put_span(span); }) / * 使用spdk调度,写入页面 * /
        >> pump::flat()                                                           / * 运行返回的执行计划 * /
        >> pump::all([](data::write_span& span){ return span.all_wrote(); })      / * 需要所有的写操作成功 * /
        >> pump::then([](bool b){ if (!b) throw data::write_data_failed(); });    / * 否则抛出异常 * /
}
...

auto
apply() {
    return pump::get_context<data::batch*>()                                      / * 从上下文中获取到batch指针 * /
        >> pump::then([](data::batch* b){
            return pump::just()                                                   / * 一个新的执行计划树 * /
                >> request_put_serial_number(b)                                   / * 获取写序列号 * /
                >> update_index(b)                                                / * 更新索引 * /
                >> merge_batch_and_allocate_page()                                / * 合并当前所有正在运行的apply算子的batch,并且分配页面 * /
                >> pump::then([b](auto &&res) {
                                                                                  / * 如果被选为leader,则负责所有batch的写操作 (这里是编译期通过visit算子判断)* /
                    if constexpr (std::is_same_v<fs::_allocate::leader_res *, __typ__(res)>) {
                        return pump::just()
                            >> write_data(res->span_list)                         / * 写入数据 * /
                            >> write_meta(res)                                    / * 写入元数据 * /
                            >> free_page_when_error(res->span_list)               / * 出现错误时候释放页面 * /
                            >> notify_follower(res);                              / * 通知跟随者 * /
                    }
                                                                                  / * 如果是follower,则等待通知 * /
                    else if constexpr (std::is_same_v<fs::_allocate::follower_res, __typ__(res)>) {
                        return pump::just(__fwd__(res));
                    }
                    else if constexpr (std::is_same_v<fs::_allocate::failed_res, __typ__(res)>){
                        return pump::just(std::make_exception_ptr(new data::allocate_page_failed()));
                    }
                    else {
                        static_assert(false);
                    }
                })
                >> pump::flat()                                                   / * 运行返回的执行计划 * /
                >> pump::ignore_all_exception()                                   / * 忽略所有的异常 * /
                >> cache_data_if_succeed(b);
        })
        >> pump::flat()                                                           / * 运行返回的执行计划 * /;
}

...

/ * ycsb A * /
inline auto 
updt(uint64_t max) {
    return start_statistic()                                                      / * 开始统计信息 * /
        >> generate_on(any_task_scheduler(), std::views::iota(uint64_t(0), max))  / * 在任意cpu核心上生成数据 * /
        >> output_statistics_per_sec()                                            / * 输出统计信息 * /
        >> concurrent(10000)                                                      / * 并发执行(最大10000个任务) * /
        >> then([](uint64_t i) { return (spdk_get_ticks() % 100) < 50; })         / * ycsb A 50%读 50%写 * /
        >> visit()                                                                / * visit算子,在编译期处理不同的逻辑 * /
        >> then([max](auto &&res) {
            if constexpr (is_put<__typ__(res)>)
                                                                                  / * 写逻辑 * /
                return as_batch( random_kv(max) >> put() >> apply() >> statistic_put());
            else
                                                                                  / * 读逻辑 * /
                return as_batch(random_key(max) >> get() >> statistic_get());
        })
        >> then([](auto&& sender) {
            return just() >> sender;                                              / * 如果需要,可以在这里对返回的执行计划(sender)做运行期的优化 * /
        })
        >> flat()
        >> reduce()
        >> stop_statistic()
        >> output_finally_statistics();                                           / * 输出统计信息 * /
}

...

int
main(int argc, char **argv){
    start_db(argc, argv)([](){                                                    / * 初始化并启动存储 * /
        return with_context(statistic_helper(new statistic_data()), logger()) (load(max_key)) / * ycsb load * /
            >> with_context(statistic_helper(new statistic_data()), logger()) (updt(max_key)) / * ycsb A * /
            >> with_context(statistic_helper(new statistic_data()), logger()) (read(max_key)) / * ycsb C * /
            >> with_context(statistic_helper(new statistic_data()), logger()) (scan(max_key)) / * ycsb E * /
            >> stop_db();
    });
    return 0;
}
```
