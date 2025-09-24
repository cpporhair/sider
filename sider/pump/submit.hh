//
//
//

#ifndef PUMP_SUBMIT_HH
#define PUMP_SUBMIT_HH

#include "./bind_back.hh"
#include "./any_context.hh"
#include "./null_receiver.hh"
#include "./op_pusher.hh"
#include "./op_poller.hh"
#include "./just.hh"

namespace pump {

    namespace _submit {
        template<typename sender_t, typename receiver_t>
        auto
        connect(sender_t &&sender, receiver_t &&receiver) {
            return sender.connect(std::forward<receiver_t>(receiver));
        }

        struct
        fn {
            template<typename context_t, typename sender_t, typename receiver_t>
            constexpr
            decltype(auto)
            operator ()(sender_t &&sender, context_t context, receiver_t&& receiver) const{
                auto new_scope = std::make_shared<root_scope<__typ__(sender.template connect<context_t>().push_back(__fwd__(receiver)).take())>>(
                    sender.template connect<context_t>().push_back(__fwd__(receiver)).take()
                );
                pusher::op_pusher<0, __typ__(new_scope)>::push_value(context, new_scope);
            }

            template<typename context_t, typename  receiver_t>
            constexpr
            decltype(auto)
            operator ()(context_t context, receiver_t&& receiver) const {
                return bind_back<fn, context_t, receiver_t>{
                    fn{},
                    context,
                    __fwd__(receiver)
                };
            }

            template<typename context_t, typename  bind_back_t>
            requires bind_back_t::bind_back_flag
            constexpr
            decltype(auto)
            operator ()(context_t context, bind_back_t&& bb) const {
                return just() >> __fwd__(bb) >> this->operator()(context);
            }

            template<typename context_t>
            constexpr
            decltype(auto)
            operator ()(context_t context) const {
                return this->operator()(context, null_receiver{});
            }
        };
    }

    inline constexpr _submit::fn submit{};
}
#endif //PUMP_SUBMIT_HH
