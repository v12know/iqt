#ifndef _IQT_SERVICE_ORDER_SERVICE_HPP_
#define _IQT_SERVICE_ORDER_SERVICE_HPP_

#include "model/order.h"
#include "model/trade.h"
#include "dao/order_dao.hpp"
//#include "base/redis.h"


namespace iqt {
/*    namespace cpp_redis {
        class client;
    }*/
    namespace service {
//    using namespace base::redis;
        using namespace model;
        using namespace dao;

        class OrderService {
        public:
            OrderPtr get_order(cpp_redis::client &r, const std::string &order_key) {
                OrderPtr ord = nullptr;
                auto iter = order_map_.find(order_key);
                if (iter != order_map_.cend()) {
                    ord = iter->second;
                } else {
                    ord = order_dao_.get_order(r, order_key);
//                PPK_ASSERT_ERROR(ord, "can't find order on redis, order_key=%s", order_key.c_str());
                    assert(ord);
                    order_map_[ord->order_key] = ord;
                }
                return ord;
            }

            void get_order(cpp_redis::client &r, OrderPtr &ord) {
                auto iter = order_map_.find(ord->order_key);
                if (iter != order_map_.cend()) {
                    ord = iter->second;
                } else {
                    order_dao_.get_order(r, *ord);
//                PPK_ASSERT_ERROR(ord, "can't find order on redis, order_key=%s", ord->order_key.c_str());
                    assert(ord);
                    order_map_[ord->order_key] = ord;
                }
            }

            void add_order(cpp_redis::client &r, OrderPtr ord) {
                ord->create_time = base::strftimep();
                ord->trading_date = Env::instance()->trading_date;
                order_map_[ord->order_key] = ord;
                order_dao_.add_order(r, *ord);
            }

            void update_order(cpp_redis::client &r, Order &ord) {
                order_dao_.update_order(r, ord);
            }

            void update_order_after_trade(cpp_redis::client &r, TradeTuple &trade) {
                order_dao_.update_order_after_trade(r, *std::get<1>(trade));
            }

        private:
            OrderDao order_dao_;
            std::unordered_map<std::string, OrderPtr> order_map_;
        };

    } /* service */
} /* iqt */
#endif /* end of include guard: _IQT_SERVICE_ORDER_SERVICE_HPP_ */
