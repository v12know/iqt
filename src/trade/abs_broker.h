#ifndef _IQT_TRADE_ABS_BROKER_H_
#define _IQT_TRADE_ABS_BROKER_H_

#include <memory>
#include <list>

//#include "model/order.h"
//#include "trade/portfolio.h"

namespace iqt {
    namespace model {
        struct Order;
    }
    namespace trade {
        class Portfolio;

        class AbstractBroker {
        public:
            virtual std::list<std::shared_ptr<model::Order>> &get_open_orders() = 0;

            virtual void submit_order(std::shared_ptr<model::Order> order) = 0;

            virtual void cancel_order(std::shared_ptr<model::Order> order) = 0;

            virtual std::shared_ptr<trade::Portfolio> get_portfolio() = 0;

            virtual std::string get_account_id() const = 0;
        };
    } /* trade */
} /* iqt */
#endif /* end of include guard: _IQT_TRADE_ABS_BROKER_H_ */
