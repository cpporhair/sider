//
//
//

#ifndef PUMP_BIND_BACK_HH
#define PUMP_BIND_BACK_HH

#include <variant>
#include <tuple>
#include "./meta.hh"

namespace pump {
    namespace _bind_back {
        template <typename ...bind_backs_t>
        struct
        bind_back_tuple {
            using bind_back_flag = std::monostate;
            std::tuple<bind_backs_t...> t{};

            explicit
            bind_back_tuple(std::tuple<bind_backs_t...>&& t0)
                :t(__fwd__(t0)){
            }

            template <typename bind_back_t>
            auto
            push_front(bind_back_t&& fn) {
                return bind_back_tuple<bind_back_t, bind_backs_t...>{std::tuple_cat(std::make_tuple(__fwd__(fn)), __mov__(t))};
            }

            template <typename sender_t>
            static
            auto
            impl (sender_t&& sender,bind_back_tuple&& fn){
                return std::apply(
                    [sender = __fwd__(sender)](auto&& ...args) mutable {
                        return (__fwd__(sender) >> ... >> __fwd__(args));
                    },
                    __fwd__(fn.t)
                );
            }

            template <typename sender_t>
            requires std::is_same_v<typename sender_t::bind_back_flag,typename sender_t::bind_back_flag>
            static
            auto
            impl (sender_t&& sender,bind_back_tuple&& fn) {
                return fn.push_front(__fwd__(sender));
            }

            template <typename sender_t>
            friend
            auto
            operator >> (sender_t&& sender,bind_back_tuple&& fn){
                return impl(__fwd__(sender), __fwd__(fn));
            }
        };

        template <typename cpo_t,typename ...args_t>
        struct
        fn {
            cpo_t cpo;
            std::tuple<args_t...> tuple_args{};

            using bind_back_flag = std::monostate;

            explicit
            fn(cpo_t&& c,args_t ...a)
                :cpo(std::forward<cpo_t>(c))
                ,tuple_args(__fwd__(a)...) {
            }

            template <typename sender_t>
            static
            auto
            impl (sender_t&& sender,fn&& fn){
                __must_rval__(sender);
                __must_rval__(fn);
                return std::apply(
                    [cpo = __mov__(fn.cpo),sender = __fwd__(sender)](args_t&& ...args) mutable {
                        return cpo(__fwd__(sender), __fwd__(args)...);
                    },
                    __fwd__(fn.tuple_args)
                );
            }

            template <typename sender_t>
            requires std::is_same_v<typename std::decay_t<sender_t>::bind_back_flag,typename std::decay_t<sender_t>::bind_back_flag>
            static
            auto
            impl (sender_t&& sender,fn&& fn) {
                return bind_back_tuple{std::make_tuple(__fwd__(sender), __fwd__(fn))};
            }


            template <typename sender_t>
            friend
            auto
            operator >> (sender_t&& sender,fn&& fn){
                __must_rval__(sender);
                __must_rval__(fn);
                return impl<sender_t>(__fwd__(sender), __fwd__(fn));
            }

            friend
            auto
            operator -- (fn&& fn){
                __must_rval__(fn);
                return __fwd__(fn);
            }

        };
    }
    template<typename cpo_t, typename ...args_t>
    using bind_back = _bind_back::fn<cpo_t, args_t...>;
}
#endif //PUMP_BIND_BACK_HH
