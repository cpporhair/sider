//

//

#ifndef SIDER_KV_DATA_EXCEPTIONS_HH
#define SIDER_KV_DATA_EXCEPTIONS_HH

#include <exception>

namespace sider::kv::data {
    struct
    absolutely_not_code_block : std::logic_error {
        absolutely_not_code_block()
            : std::logic_error("absolutely_not_code_block"){
        }

        absolutely_not_code_block(absolutely_not_code_block&& rhs)
            : std::logic_error(__fwd__(rhs)){
        }
    };

    struct
    allocate_page_failed : std::logic_error {
        allocate_page_failed()
            : std::logic_error("allocate_page_failed"){
        }

        allocate_page_failed(allocate_page_failed&& rhs)
            : std::logic_error(__fwd__(rhs)){
        }
    };

    struct
    update_index_failed : std::logic_error {
        update_index_failed()
            : std::logic_error("update_index_failed"){
        }

        update_index_failed(update_index_failed&& rhs)
            : std::logic_error(__fwd__(rhs)){
        }
    };

    struct
    write_data_failed : std::logic_error {
        write_data_failed()
            : std::logic_error("write_data_failed"){
        }

        write_data_failed(write_data_failed&& rhs)
            : std::logic_error(__fwd__(rhs)){
        }
    };

    struct
    read_page_failed : std::logic_error {
        read_page_failed()
            : std::logic_error("read_page_failed"){
        }

        read_page_failed(read_page_failed&& rhs)
            : std::logic_error(__fwd__(rhs)){
        }

        read_page_failed(const read_page_failed& rhs)
            : std::logic_error(rhs) {
        }
    };

    struct
    apply_batch_failed : std::logic_error {
        apply_batch_failed()
            : std::logic_error("apply_batch_failed"){
        }

        apply_batch_failed(apply_batch_failed&& rhs)
            : std::logic_error(__fwd__(rhs)){
        }
    };

    struct
    init_env_failed : std::logic_error {
        init_env_failed(const char* msg)
            : std::logic_error(msg){
        }

        init_env_failed(init_env_failed&& rhs)
            : std::logic_error(__fwd__(rhs)){
        }
    };
}

#endif //SIDER_KV_DATA_EXCEPTIONS_HH
