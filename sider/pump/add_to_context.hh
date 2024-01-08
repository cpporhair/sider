//
//
//

#ifndef SIDER_PUMP_ADD_TO_CONTEXT_HH
#define SIDER_PUMP_ADD_TO_CONTEXT_HH

namespace sider::pump {
    namespace _add_to_context {
        template <typename ...content_t>
        struct
        op {
            std::tuple<content_t...> contents;

            op(std::tuple<content_t...>&& c)
                : contents(__fwd__(c)){
            }

            op(op &&rhs)
                : contents(__fwd__(rhs.contents)){
            }

            op(const op &rhs)
                : contents(rhs.contents) {
            }

            template <uint64_t id, typename context_t>
            auto
            get_new_context(context_t& context) {
                return std::make_shared<pushed_context<id, context_t, content_t...>>(context, contents);
            }
        };

    }
}

#endif //SIDER_PUMP_ADD_TO_CONTEXT_HH
