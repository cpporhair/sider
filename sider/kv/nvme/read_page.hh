//

//

#ifndef SIDER_KV_NVME_READ_PAGE_HH
#define SIDER_KV_NVME_READ_PAGE_HH

#include "./runtime.hh"
#include "./chose_scheduler.hh"

namespace sider::kv::nvme {
    auto
    get_page(data::data_page* p) {
        return chose_scheduler(p->ssd_index)->get(p);
    }
}

#endif //SIDER_KV_NVME_READ_PAGE_HH
