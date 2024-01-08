//

//

#ifndef SIDER_KV_NVME_PUT_PAGE_HH
#define SIDER_KV_NVME_PUT_PAGE_HH

#include "sider/pump/for_each.hh"
#include "sider/pump/visit.hh"
#include "sider/pump/concurrent.hh"

#include "./scheduler.hh"
#include "./runtime.hh"
#include "./chose_scheduler.hh"

namespace sider::kv::nvme {

    auto
    put_span(data::write_span& span) {
        return chose_scheduler(span.ssd_index)->put(span);
    }
}

#endif //SIDER_KV_NVME_PUT_PAGE_HH
