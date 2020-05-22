#include "trade/api_future.h"

#include "model/tick.h"
#include "trade/portfolio.h"
#include "trade/abs_broker.h"
#include "env.h"
#include "base/log.h"

namespace iqt {
    namespace trade {

        std::vector<std::shared_ptr<model::Order>>
        ApiFuture::order(const std::string &order_book_id, int amount, const trade::SIDE_TYPE &side,
                         const trade::POSITION_EFFECT_TYPE &position_effect, double price,
                         const trade::ORDER_TYPE &type) {
            auto position = Env::instance()->portfolio->positions()->get_or_create(order_book_id);
            std::vector<std::shared_ptr<model::Order>> ord_vec;
            switch (position_effect) {
                case trade::POSITION_EFFECT_TYPE::CLOSE_TODAY:
                case trade::POSITION_EFFECT_TYPE::CLOSE: {
                    POSITION_EFFECT_TYPE first_pos_eff, second_pos_eff;
                    int first_sell_quantity, first_buy_quantity;
                    if (trade::POSITION_EFFECT_TYPE::CLOSE_TODAY == position_effect) {
                        first_pos_eff = POSITION_EFFECT_TYPE::CLOSE_TODAY;
                        second_pos_eff = POSITION_EFFECT_TYPE::CLOSE;
                        first_sell_quantity = position->sell_today_quantity();
                        first_buy_quantity = position->buy_today_quantity();
                    } else {
                        first_pos_eff = POSITION_EFFECT_TYPE::CLOSE;
                        second_pos_eff = POSITION_EFFECT_TYPE::CLOSE_TODAY;
                        first_sell_quantity = position->sell_old_quantity();
                        first_buy_quantity = position->buy_old_quantity();
                    }
                    if (side == trade::SIDE_TYPE::BUY) {
                        // 如果平仓量大于持仓量，则 Warning 并 取消订单创建
                        if (amount > position->sell_quantity()) {
                            log_warn("Order Creation Failed: close amount {0} is larger than position quantity {1}",
                                     amount,
                                     position->sell_quantity());
                            return ord_vec;
                        }
                        if (amount > first_sell_quantity) {
                            if (first_sell_quantity > 0) {
                                //如果有昨仓，则创建一个 POSITION_EFFECT.CLOSE 的平仓单
                                ord_vec.emplace_back(
                                        std::make_shared<model::Order>(order_book_id, first_sell_quantity, side, type,
                                                                       first_pos_eff, price, rpt_key_));
                            }
                            //剩下还有仓位，则创建一个 POSITION_EFFECT.CLOSE_TODAY 的平今单
                            ord_vec.emplace_back(
                                    std::make_shared<model::Order>(order_book_id, amount - first_sell_quantity, side,
                                                                   type,
                                                                   second_pos_eff, price, rpt_key_));
                        } else {
                            //创建 POSITION_EFFECT.CLOSE 的平仓单
                            ord_vec.emplace_back(std::make_shared<model::Order>(order_book_id, amount, side, type,
                                                                                first_pos_eff, price,
                                                                                rpt_key_));
                        }
                    } else {
                        // 如果平仓量大于持仓量，则 Warning 并 取消订单创建
                        if (amount > position->buy_quantity()) {
                            log_warn("Order Creation Failed: close amount {0} is larger than position quantity {1}",
                                     amount,
                                     position->buy_quantity());
                            return ord_vec;
                        }
                        if (amount > first_buy_quantity) {
                            if (first_buy_quantity > 0) {
                                //如果有昨仓，则创建一个 POSITION_EFFECT.CLOSE 的平仓单
                                ord_vec.emplace_back(
                                        std::make_shared<model::Order>(order_book_id, first_buy_quantity, side, type,
                                                                       first_pos_eff, price, rpt_key_));
                            }
                            //剩下还有仓位，则创建一个 POSITION_EFFECT.CLOSE_TODAY 的平今单
                            ord_vec.emplace_back(
                                    std::make_shared<model::Order>(order_book_id, amount - first_buy_quantity, side,
                                                                   type,
                                                                   second_pos_eff, price, rpt_key_));
                        } else {
                            //创建 POSITION_EFFECT.CLOSE 的平仓单
                            ord_vec.emplace_back(std::make_shared<model::Order>(order_book_id, amount, side, type,
                                                                                first_pos_eff, price,
                                                                                rpt_key_));
                        }

                    }
                    break;
                }
                case POSITION_EFFECT_TYPE::OPEN: {
                    ord_vec.emplace_back(
                            std::make_shared<model::Order>(order_book_id, amount, side, type, position_effect, price,
                                                           rpt_key_));
                    break;
                }
                default:;
            }
            for (auto &o : ord_vec) {
                if (o->type == ORDER_TYPE::MARKET) {
                    auto tick = Env::instance()->snapshot[order_book_id];
                    if (!tick) {
                        log_warn("Order Creation Failed: [{0}] No market data", order_book_id);
                        for (auto &o : ord_vec) {
                            o->mark_rejected("Order Creation Failed: [" + order_book_id + "] No market data");
                        }
                        return ord_vec;
                    }
                    o->price = tick->last;
                }
                if (Env::instance()->can_submit_order(o)) {
                    Env::instance()->broker->submit_order(o);
                }
            }
            return ord_vec;
        }

    } /* trade */
} /* iqt */
