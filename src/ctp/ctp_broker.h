#ifndef _IQT_CTP_CTP_BROKER_H_
#define _IQT_CTP_CTP_BROKER_H_

#include "trade/abs_broker.h"
#include "ctp/trade_gateway.h"

namespace iqt {
/*    namespace model {
        struct Order;
        struct Portfolio;
    }*/
    namespace trade {

//        class CtpTradeGateway;

        class CtpBroker : public trade::AbstractBroker {

        public:
            CtpBroker(CtpTradeGateway *trade_gateway) : trade_gateway_(trade_gateway) {}

            std::list<std::shared_ptr<model::Order>> &get_open_orders() {
                return trade_gateway_->open_orders();
            }

            void submit_order(std::shared_ptr<model::Order> order) {
                trade_gateway_->submit_order(order);
            }

            void cancel_order(std::shared_ptr<model::Order> order) {
                trade_gateway_->cancel_order(order);
            }

            std::shared_ptr<trade::Portfolio> get_portfolio() {
                return trade_gateway_->get_portfolio();
            }

            std::string get_account_id() const {
                return trade_gateway_->get_account_id();
            }
        private:
            CtpTradeGateway *trade_gateway_;

        };

    } /* ctp */
}/* iqt */
#endif /* end of include guard: _IQT_CTP_CTP_BROKER_H_ */
