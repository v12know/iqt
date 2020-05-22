#ifndef _IQT_DAO_ORDER_DAO_HPP_
#define _IQT_DAO_ORDER_DAO_HPP_

#include "dao/base_dao.hpp"
#include "model/order.h"
#include "base/util.hpp"

namespace iqt {
    namespace dao {
        class OrderDao : public BaseDao {
        public:
            OrderPtr get_order(cpp_redis::client &r, const std::string &order_key) {

                auto ord = std::make_shared<Order>();
                get_order(r, *ord);
                return ord;
            }

            void get_order(cpp_redis::client &r, Order &ord) {
                auto hmget = r.hmget(ord.order_key, order_fields_);
                r.sync_commit();

                auto array = hmget.get().as_array();
                ord.order_book_id = array[0].as_string();
                ord.side = trade::s_side_type.literal2enum(array[1].as_string());
                ord.position_effect = trade::s_position_effect_type.literal2enum(array[2].as_string());
                ord.status = trade::s_order_status_type.literal2enum(array[3].as_string());
                ord.avg_price = std::stod(array[4].as_string());
                ord.filled_quantity = std::stoi(array[5].as_string());
                ord.price = std::stod(array[6].as_string());
                ord.quantity = std::stoi(array[7].as_string());
                ord.type = trade::s_order_type.literal2enum(array[8].as_string());
                ord.order_sys_key = array[9].as_string();
                ord.group_key = array[10].as_string();
                ord.rpt_key = array[11].as_string();
                ord.transaction_cost = std::stod(array[12].as_string());
            }

            void add_order(cpp_redis::client &r, Order &ord) {
                r.hmset(ord.order_key, {
                        {order::ORDER_BOOK_ID, ord.order_book_id},
                        {order::CREATE_TIME, ord.create_time},
                        {order::TRADING_DATE, ord.trading_date},
                        {order::SIDE, trade::s_side_type.enum2literal(ord.side)},
                        {order::POSITION_EFFECT, trade::s_position_effect_type.enum2literal(ord.position_effect)},
                        {order::STATUS, trade::s_order_status_type.enum2literal(ord.status)},
                        {order::AVG_PRICE, base::to_string(ord.avg_price)},
                        {order::FILLED_QUANTITY, std::to_string(ord.filled_quantity)},
                        {order::PRICE, base::to_string(ord.price)},
                        {order::QUANTITY, std::to_string(ord.quantity)},
                        {order::TYPE, trade::s_order_type.enum2literal(ord.type)},
                        {order::ORDER_SYS_KEY, ord.order_sys_key},
                        {order::GROUP_KEY, ord.group_key},
                        {order::RPT_KEY, ord.rpt_key},
                        {order::TRANSACTION_COST, base::to_string(ord.transaction_cost)}
                });
            }

            void update_order(cpp_redis::client &r, Order &ord) {
                r.hmset(ord.order_key, {
                        {order::STATUS, trade::s_order_status_type.enum2literal(ord.status)},
                        {order::FILLED_QUANTITY, std::to_string(ord.filled_quantity)},
                        {order::ORDER_SYS_KEY, ord.order_sys_key}
                });
            }

            void update_order_after_trade(cpp_redis::client &r, Order &ord) {
                r.hmset(ord.order_key, {
                        {order::AVG_PRICE, base::to_string(ord.avg_price)},
                        {order::TRANSACTION_COST, base::to_string(ord.transaction_cost)}
                });
            }

        private:
            std::vector<std::string> order_fields_ = {order::ORDER_BOOK_ID, order::SIDE, order::POSITION_EFFECT, order::STATUS, order::AVG_PRICE,
                                                      order::FILLED_QUANTITY, order::PRICE, order::QUANTITY, order::TYPE, order::ORDER_SYS_KEY, order::GROUP_KEY,
                                                      order::RPT_KEY, order::TRANSACTION_COST};
        };
    } /* dao  */

} /* iqt */

#endif /* end of include guard: _IQT_DAO_ORDER_DAO_HPP_ */
