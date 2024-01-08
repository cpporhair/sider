//

//

#ifndef SIDER_KV_BATCH_REQUEST_READ_TASK_HH
#define SIDER_KV_BATCH_REQUEST_READ_TASK_HH


namespace sider::kv::batch {
    struct
    request_get_id_task {
        std::move_only_function<void(data::snapshot*, uint64_t)> cb{};
    };

    struct
    request_put_id_task {
        std::move_only_function<void(data::snapshot*, uint64_t)> cb{};
    };
}

#endif //SIDER_KV_BATCH_REQUEST_READ_TASK_HH
