//

//

#ifndef SIDER_KV_RUNTIME_CONFIG_HH
#define SIDER_KV_RUNTIME_CONFIG_HH

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>

namespace sider::kv::runtime {
    std::vector<uint32_t> batch_used_core{0};
    std::vector<uint32_t> index_used_core{1, 2, 3};
    std::vector<uint32_t> fs_used_core{0};
    std::vector<uint32_t> io_used_core{4, 5, 6, 7, 8, 9};

    struct
    batch_config {
        std::vector<uint32_t>       used_core;
    };

    struct
    index_config {
        std::vector<uint32_t>       used_core;
    };

    struct
    fs_config {
        std::vector<uint32_t>       used_core;
    };

    struct
    nvme_config {
        std::vector<uint32_t>       used_core;
        uint32_t                    fs_core;
        std::string                 name;
        uint32_t                    qpair_count;
        uint32_t                    qpair_depth;
        bool                        fua;
    };

    struct
    io_config {
        std::vector<nvme_config>    ssds;

        bool
        has_core(uint32_t c) {
            return std::find_if(ssds.begin(), ssds.end(),[c](const nvme_config& config){
                if(config.fs_core == c)
                    return true;
                else
                    return std::find(config.used_core.begin(), config.used_core.end(), c) != config.used_core.end();
            })!= ssds.end();
        }
    };

    struct
    db_config {
        std::string                 name;
        std::string                 meta_file_path;
        batch_config                batch;
        index_config                index;
        fs_config                   fs;
        io_config                   io;

        auto
        compute_core_mask() {
            uint64_t mask = 0;
            for (uint32_t core = 0; core < std::thread::hardware_concurrency(); ++core) {
                if (std::find(batch.used_core.begin(), batch.used_core.end(), core) != batch.used_core.end())
                    mask |= 1 << core;
                else if (std::find(index.used_core.begin(), index.used_core.end(), core) != index.used_core.end())
                    mask |= 1 << core;
                else if (std::find(fs.used_core.begin(), fs.used_core.end(), core) != fs.used_core.end())
                    mask |= 1 << core;
                else if (io.has_core(core))
                    mask |= 1 << core;
            }
            return mask;
        }
    };

    db_config config;

    auto
    read_config_from_file(db_config& c, std::string file_path) {
        boost::property_tree::ptree pt;
        boost::property_tree::read_json(file_path, pt);

        auto batch_config = pt.get_child("batch");

        BOOST_FOREACH(boost::property_tree::ptree::value_type &v, batch_config.get_child("core")) {
            c.batch.used_core.push_back(stoi(v.second.data()));
        }

        auto index_config = pt.get_child("index");

        BOOST_FOREACH(boost::property_tree::ptree::value_type &v, index_config.get_child("core")) {
            c.index.used_core.push_back(stoi(v.second.data()));
        }

        auto fs_config = pt.get_child("fs");

        BOOST_FOREACH(boost::property_tree::ptree::value_type &v, fs_config.get_child("core")) {
            c.fs.used_core.push_back(stoi(v.second.data()));
        }

        auto io_config = pt.get_child("io");

        BOOST_FOREACH(boost::property_tree::ptree::value_type &v, io_config.get_child("nvme")) {
            nvme_config nvcfg;
            nvcfg.fs_core = v.second.get<uint32_t>("page_manager");
            nvcfg.name = v.second.get<std::string>("name");
            nvcfg.fua = v.second.get<bool>("fua");
            nvcfg.qpair_count = v.second.get<uint32_t>("qpair_count");
            nvcfg.qpair_depth = v.second.get<uint32_t>("qpair_depth");
            BOOST_FOREACH(boost::property_tree::ptree::value_type &v1, v.second.get_child("core")) {
                nvcfg.used_core.push_back(stoi(v1.second.data()));
            }
            c.io.ssds.push_back(nvcfg);
        }
    }

    template<typename T>
    void update_maximum(std::atomic<T>& maximum_value, T const& value) noexcept
    {
        T prev_value = maximum_value;
        while(prev_value < value &&
              !maximum_value.compare_exchange_weak(prev_value, value))
        {}
    }

    uint32_t main_core = 0;
    bool running = true;

    struct
    task_info {
        uint64_t done_task_count[1024];
        std::atomic<uint64_t> page_count;
        std::atomic<uint64_t> start_batch_count;
        std::atomic<uint64_t> index_count;
        std::atomic<uint64_t> fs_count;
        std::atomic<uint64_t> nv_count;
        std::atomic<uint64_t> batch_count;
        std::atomic<uint64_t> gen_count;
        std::atomic<uint64_t> wait_batch;
        std::atomic<uint64_t> publish;
    };

    task_info g_task_info;

}

#endif //SIDER_KV_RUNTIME_CONFIG_HH
