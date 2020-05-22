#ifndef _IQT_TRADE_API_FUTURE_H_
#define _IQT_TRADE_API_FUTURE_H_

#include <string>
#include <memory>
#include <vector>

#include "const.hpp"

namespace iqt {
    namespace model {
        struct Order;
    }
    namespace trade {
        class ApiFuture {
        public:
            void set_rpt_key(const std::string &rpt_key) {
                rpt_key_ = rpt_key;
            }

            std::vector<std::shared_ptr<model::Order>>
            order(const std::string &order_book_id, int amount, const trade::SIDE_TYPE &side,
                  const trade::POSITION_EFFECT_TYPE &position_effect, double price, const trade::ORDER_TYPE &type);

            std::vector<std::shared_ptr<model::Order>>
            buy_open(const std::string &order_book_id, int amount, double price,
                     const trade::ORDER_TYPE &type = trade::ORDER_TYPE::FAK) {
                return order(order_book_id, amount, trade::SIDE_TYPE::BUY, trade::POSITION_EFFECT_TYPE::OPEN, price,
                             type);
            }

            std::vector<std::shared_ptr<model::Order>>
            buy_close(const std::string &order_book_id, int amount, double price, bool close_today = false,
                      const trade::ORDER_TYPE &type = trade::ORDER_TYPE::FAK) {
                return order(order_book_id, amount, trade::SIDE_TYPE::BUY,
                             close_today ? POSITION_EFFECT_TYPE::CLOSE_TODAY : POSITION_EFFECT_TYPE::CLOSE, price,
                             type);
            }

            std::vector<std::shared_ptr<model::Order>>
            sell_open(const std::string &order_book_id, int amount, double price,
                      const trade::ORDER_TYPE &type = trade::ORDER_TYPE::FAK) {
                return order(order_book_id, amount, trade::SIDE_TYPE::SELL, trade::POSITION_EFFECT_TYPE::OPEN, price,
                             type);
            }

            std::vector<std::shared_ptr<model::Order>>
            sell_close(const std::string &order_book_id, int amount, double price, bool close_today = false,
                       const trade::ORDER_TYPE &type = trade::ORDER_TYPE::FAK) {
                return order(order_book_id, amount, trade::SIDE_TYPE::SELL,
                             close_today ? POSITION_EFFECT_TYPE::CLOSE_TODAY : POSITION_EFFECT_TYPE::CLOSE, price,
                             type);
            }

        private:
            std::string rpt_key_;
        };
    } /* trade */
} /* iqt */
#endif /* end of include guard: _IQT_TRADE_API_FUTURE_H_ */
