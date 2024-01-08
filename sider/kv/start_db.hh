//

//

#ifndef SIDER_KV_START_DB_HH
#define SIDER_KV_START_DB_HH

#include <filesystem>

#include "3rd/popl/popl.hh"

#include "sider/pump/generate.hh"
#include "sider/pump/repeat.hh"
#include "sider/pump/get_context.hh"
#include "sider/pump/any_exception.hh"
#include "sider/pump/pop_context.hh"
#include "sider/pump/push_context.hh"
#include "sider/task/any.hh"

#include "./data/compute_key_seed.hh"
#include "./runtime/scheduler_objects.hh"
#include "./runtime/config.hh"
#include "./runtime/nvme.hh"
#include "./runtime/init.hh"
#include "./runtime/fs.hh"


#include "./as_task.hh"

namespace sider::kv {

    auto
    init_proc_env() {
        return pump::then([](){ runtime::init_proc_env(); });
    }

    auto
    init_nvme_env() {
        return pump::then([](){ runtime::init_ssd(); });
    }

    auto
    save_index() {
        return pump::then([](...){
            auto *file = std::fopen(("./index.db.tmp"), "w+");
            uint64_t len = 0;
            std::fwrite(&len, 1, sizeof(uint64_t), file);
            for(auto* btree : runtime::all_index.list){
                for(auto i = btree->impl.begin(); i!= btree->impl.end(); ++i) {
                    if (i->second->versions.size() > 0) {
                        len++;
                        if (i->second->key->len >= 100) {
                            std::cout << "i->second->key->len<100 " << i->second->key->ptr << std::endl;
                            SPDK_ERRLOG("i->second->key->len<100 %ld \n", i->second->key->len);
                        }


                        std::fwrite(i->second->key, 1, i->second->key->len, file);
                        data::version *v = *i->second->versions.begin();
                        std::fwrite(&v->sn, 1, sizeof(uint64_t), file);
                        uint64_t page_count = v->file->pages.size();
                        std::fwrite(&page_count, 1, sizeof(uint64_t), file);
                        for (auto* page : v->file->pages) {
                            std::fwrite(page->ssd_sn, 1, 20, file);
                            std::fwrite(&page->pos, 1, sizeof(uint64_t), file);
                        }
                    }
                }
            }
            std::fseek(file, 0, 0);
            std::fwrite(&len, 1, sizeof(uint64_t), file);
            std::fflush(file);
            std::fclose(file);
            std::filesystem::copy("./index.db.tmp","./index.db");
            std::filesystem::remove("./index.db.tmp");
            std::cout << "save pages = " << len << std::endl;
        });
    }

    auto
    init_indexes() {
        return pump::get_context<popl::option_parser*>()
            >> pump::then([](popl::option_parser* c){
                runtime::init_indexes();
                if (!std::filesystem::exists("./index.db"))
                    return;
                auto *file = std::fopen(("./index.db"), "r");
                uint64_t len = 0;
                std::fread(&len,1, sizeof(uint64_t), file);
                std::cout << "read pages = " << len << std::endl;
                for (uint64_t i = 0; i < len; ++i) {
                    auto* f = new data::data_file;

                    uint64_t key_len = 0;
                    std::fread(&key_len, 1, sizeof(uint64_t), file);
                    assert(key_len<100);
                    auto* key = (data::slice*) malloc(key_len);
                    key->len = key_len;
                    std::fread(key->ptr, 1, key_len - sizeof(uint64_t), file);

                    uint64_t sn = 0;
                    std::fread(&sn, 1, sizeof(uint64_t), file);

                    auto *new_index = new data::index(key, new data::version(sn));
                    new_index->key = key;

                    uint64_t page_count = 0;
                    std::fread(&page_count, 1, sizeof(uint64_t), file);
                    for (uint64_t j = 0; j < page_count; ++j) {
                        auto* page = new data::data_page;
                        char* ssd_sn = new char[20];
                        std::fread(ssd_sn, 1, 20, file);
                        auto* ssd = runtime::ssds.find_ssd_by_sn(ssd_sn);
                        page->flg = 0;
                        page->payload = nullptr;
                        page->ssd_sn = ssd->sn;
                        page->ssd_index = ssd->index_on_config;
                        std::fread(&page->pos, 1, sizeof(uint64_t), file);
                        (*new_index->versions.begin())->file->pages.emplace_back(page);
                    }

                    runtime::all_index.list[data::compute_key_seed(new_index->key) % runtime::config.index.used_core.size()]
                        ->impl.emplace(new_index->key, new_index);
                }
                std::fclose(file);
                std::filesystem::remove("./index.db");
            });
    }

    auto
    init_pages() {
        return pump::then([](){
            runtime::init_allocator();
            for (auto *btree: runtime::all_index.list) {
                for (auto & i : btree->impl) {
                    for (auto *v: i.second->versions) {
                        for (auto *p: v->file->pages) {
                            auto *ssd = runtime::ssds.find_ssd_by_sn(p->ssd_sn);
                            ssd->allocator->set_page_allocated(p->pos);
                        }
                    }
                }
            }

            for (auto *ssd: runtime::ssds.ssds) {
                ssd->allocator->init();
            }
        });
    };

    auto
    init_schedulers() {
        return pump::get_context<popl::option_parser*>()
            >> pump::then([](popl::option_parser* c){
                runtime::init_schedulers();
            });
    }

    int
    task_proc(void* arg) {
        uint32_t this_core = spdk_env_get_current_core();
        std::vector<std::move_only_function<void()const>> advance_list;

        advance_list.emplace_back([this_core]() { runtime::task_schedulers.by_core[this_core]->advance(); });

        if(runtime::batch_schedulers.by_core[this_core])
            advance_list.emplace_back([this_core]() { runtime::batch_schedulers.by_core[this_core]->advance(); });
        if(runtime::index_schedulers.by_core[this_core])
            advance_list.emplace_back([this_core]() { runtime::index_schedulers.by_core[this_core]->advance(); });
        if(runtime::fs_schedulers.by_core[this_core])
            advance_list.emplace_back([this_core]() { runtime::fs_schedulers.by_core[this_core]->advance(); });

        for (auto &schedulers : runtime::nvme_schedulers_per_ssd)
            if (schedulers.by_core[this_core])
                advance_list.emplace_back([this_core, schedulers]() { schedulers.by_core[this_core]->advance(); });

        while(runtime::running)
            std::ranges::for_each(advance_list, [](auto &f) { f(); });
        return 0;
    }

    auto
    run_schedule_proc() {
        return pump::then([](){
            runtime::main_core = spdk_env_get_current_core();
            for (auto c = spdk_env_get_first_core(); c != UINT32_MAX; c = spdk_env_get_next_core(c)){
                if(c != spdk_env_get_current_core()) {
                    spdk_env_thread_launch_pinned(c, task_proc, nullptr);
                }
            }
            task_proc(nullptr);
            spdk_env_thread_wait_all();
        });
    }

    template <typename func_t>
    auto
    run(func_t&& func) {
        return pump::then(__fwd__(func));
    }

    auto
    finally() {
        return pump::ignore_args()
            >> pump::any_exception([](std::exception_ptr p){
                try {
                    std::rethrow_exception(p);
                }
                catch (std::invalid_argument& e) {
                    std::cout << e.what() << std::endl;
                    return pump::just();
                }
                catch (std::exception& e) {
                    std::cout << e.what() << std::endl;
                    return pump::just();
                }
            })
            >> save_index();
    }

    auto
    init_config() {
        return pump::get_context<popl::option_parser*>()
            >> pump::then([](popl::option_parser* op) mutable {
                auto x = op->get_option<popl::value<std::string>>("config");
                return runtime::init_config(x->get_value());
            });
    }

    auto
    init_all() {
        return init_config()
            >> init_proc_env()
            >> init_nvme_env()
            >> init_indexes()
            >> init_pages()
            >> init_schedulers();
    }

    auto
    prepare_options(popl::option_parser& op) {
        op.add<popl::value<std::string>>("","config","config file path");
    }

    auto
    with_options(popl::option_parser* op) {
        return [op](auto &&b) mutable {
            return pump::with_context(op)(__fwd__(b));
        };
    }

    auto
    with_options(int argc, char **argv) {
        auto* op = new popl::option_parser();
        prepare_options(*op);
        op->parse(argc, argv);
        return with_options(op);
    }

    auto
    start_db(int argc, char **argv) {
        return [argc, argv](auto&& user_func){
            pump::just()
                >> with_options(argc, argv)( init_all() )
                >> pump::then([user_func = __fwd__(user_func)]() mutable {
                    return pump::just()
                        >> task::as_task()
                        >> pump::ignore_inner_exception(user_func())
                        >> pump::submit(pump::make_root_context());
                })
                >> run_schedule_proc()
                >> finally()
                >> pump::submit(pump::make_root_context());
        };
    }
}

#endif //SIDER_KV_START_DB_HH
