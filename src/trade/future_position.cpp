#include "trade/future_position.h"

#include <cassert>

#include "trade/abs_broker.h"
#include "env.h"
//#include <event/event_type.h>
#include "event/event.h"

namespace iqt {
    namespace trade {
        void FuturePosition::update_last_price() {
            auto tick = Env::instance()->snapshot[order_book_id_];
            assert(tick);
            last_price_ = tick->last;
        }

        double FuturePosition::apply_trade(std::shared_ptr<model::Trade> trade) {
            auto trade_quantity = trade->last_quantity;
            if (trade->side == trade::SIDE_TYPE::BUY) {
                if (trade->position_effect == trade::POSITION_EFFECT_TYPE::OPEN) {
                    auto buy_quantity = this->buy_quantity();
                    buy_avg_open_price_ = (buy_avg_open_price_ * buy_quantity + trade_quantity * trade->last_price) / (buy_quantity + trade_quantity);
                    buy_transaction_cost_ += trade->transaction_cost();
                    buy_today_holding_list_.push_front({trade->last_price, trade_quantity});
                    return -1 * global::margin_of(order_book_id_, trade_quantity, trade->last_price);
                } else {
                    auto old_margin = margin();
                    sell_transaction_cost_ += trade->transaction_cost();
                    auto delta_realized_pnl = _close_holding(trade);
                    sell_realized_pnl_ += delta_realized_pnl;
                    return old_margin - margin() + delta_realized_pnl;
                }
            } else {
                if (trade->position_effect == trade::POSITION_EFFECT_TYPE::OPEN) {
                    auto sell_quantity = this->sell_quantity();
                    sell_avg_open_price_ = (sell_avg_open_price_ * sell_quantity + trade_quantity * trade->last_price) / (sell_quantity + trade_quantity);
                    sell_transaction_cost_ += trade->transaction_cost();
                    sell_today_holding_list_.push_front({trade->last_price, trade_quantity});
                    return -1 * global::margin_of(order_book_id_, trade_quantity, trade->last_price);
                } else {
                    auto old_margin = margin();
                    buy_transaction_cost_ += trade->transaction_cost();
                    auto delta_realized_pnl = _close_holding(trade);
                    buy_realized_pnl_ += delta_realized_pnl;
                    return old_margin - margin() + delta_realized_pnl;
                }

            }
        }
        double FuturePosition::_close_holding(std::shared_ptr<model::Trade> trade) {
            auto left_quantity = trade->last_quantity;
            double delta = 0;
            int consumed_quantity = 0;
            if (trade->side == trade::SIDE_TYPE::BUY) {
                // 先平昨仓
                if (sell_old_holding_list_.size() > 0) {
                    auto &pair = sell_old_holding_list_.back();
                    double old_price = pair.first;
                    int old_quantity = pair.second;
                    sell_old_holding_list_.pop_back();
                    if (old_quantity > left_quantity) {
                        consumed_quantity = left_quantity;
                        sell_old_holding_list_.push_back({old_price, old_quantity - left_quantity});
                    } else {
                        consumed_quantity = old_quantity;
                    }
                    left_quantity -= consumed_quantity;
                    delta += _cal_realized_pnl(old_price, trade->last_price, trade->side, consumed_quantity);
                }
                // 再平今仓
                while (true) {
                    if (left_quantity <= 0) {
                        break;
                    }
                    auto &pair = sell_today_holding_list_.back();
                    double oldest_price = pair.first;
                    int oldest_quantity = pair.second;
                    sell_today_holding_list_.pop_back();
                    if (oldest_quantity > left_quantity) {
                        consumed_quantity = left_quantity;
                        sell_today_holding_list_.emplace_back(oldest_price, oldest_quantity - left_quantity);
                    } else {
                        consumed_quantity = oldest_quantity;
                    }
                    left_quantity -= consumed_quantity;
                    delta += _cal_realized_pnl(oldest_price, trade->last_price, trade->side, consumed_quantity);
                }
            } else {
                // 先平昨仓
                if (buy_old_holding_list_.size() > 0) {
                    auto &pair = buy_old_holding_list_.back();
                    double old_price = pair.first;
                    int old_quantity = pair.second;
                    buy_old_holding_list_.pop_back();
                    if (old_quantity > left_quantity) {
                        consumed_quantity = left_quantity;
                        buy_old_holding_list_.push_back({old_price, old_quantity - left_quantity});
                    } else {
                        consumed_quantity = old_quantity;
                    }
                    left_quantity -= consumed_quantity;
                    delta += _cal_realized_pnl(old_price, trade->last_price, trade->side, consumed_quantity);
                }
                // 再平今仓
                while (true) {
                    if (left_quantity <= 0) {
                        break;
                    }
                    auto &pair = buy_today_holding_list_.back();
                    double oldest_price = pair.first;
                    int oldest_quantity = pair.second;
                    buy_today_holding_list_.pop_back();
                    if (oldest_quantity > left_quantity) {
                        consumed_quantity = left_quantity;
                        buy_today_holding_list_.emplace_back(oldest_price, oldest_quantity - left_quantity);
                    } else {
                        consumed_quantity = oldest_quantity;
                    }
                    left_quantity -= consumed_quantity;
                    delta += _cal_realized_pnl(oldest_price, trade->last_price, trade->side, consumed_quantity);
                }
            }
            return delta;
        }

        int FuturePosition::contract_multiplier() {
            return Env::instance()->get_instrument(order_book_id_)->contract_multiplier;
        }

        double FuturePosition::margin_rate() {
            auto instrument = Env::instance()->get_instrument(order_book_id_);
            return instrument->long_margin_ratio * Env::instance()->config.margin_multiplier;
        }

        std::list<std::shared_ptr<model::Order>> &FuturePosition::open_orders() {
            return Env::instance()->broker->get_open_orders();
        }
        /*
        int FuturePosition::buy_open_order_quantity() {
            int sum = 0;
            for (auto &order : open_orders()) {
                if (order->side == trade::SIDE_TYPE::BUY && order->position_effect == trade::POSITION_EFFECT_TYPE::OPEN) {
                    sum += order->quantity - order->filled_quantity;
                }
            }
            return sum;
        }
        int FuturePosition::sell_open_order_quantity() {
            int sum = 0;
            for (auto &order : open_orders()) {
                if (order->side == trade::SIDE_TYPE::SELL && order->position_effect == trade::POSITION_EFFECT_TYPE::OPEN) {
                    sum += order->quantity - order->filled_quantity;
                }
            }
            return sum;
        }
        int FuturePosition::buy_close_order_quantity() {
            int sum = 0;
            for (auto &order : open_orders()) {
                if (order->side == trade::SIDE_TYPE::BUY && (order->position_effect == trade::POSITION_EFFECT_TYPE::CLOSE_TODAY ||
                                                             order->position_effect == trade::POSITION_EFFECT_TYPE::CLOSE)) {
                    sum += order->quantity - order->filled_quantity;
                }
            }
            return sum;
        }
        int FuturePosition::sell_close_order_quantity() {
            int sum = 0;
            for (auto &order : open_orders()) {
                if (order->side == trade::SIDE_TYPE::SELL && (order->position_effect == trade::POSITION_EFFECT_TYPE::CLOSE_TODAY ||
                                                              order->position_effect == trade::POSITION_EFFECT_TYPE::CLOSE)) {
                    sum += order->quantity - order->filled_quantity;
                }
            }
            return sum;
        }*/

        std::shared_ptr<FuturePosition> &Positions::get_or_create(const std::string &order_book_id) {
            auto iter = positions_.find(order_book_id);
            if (iter != positions_.end()) {
                return iter->second;
            } else {
                std::vector<std::string> tmp_vec({order_book_id});
                auto event = std::make_shared<event::Event<std::vector<std::string> &>>(event::POST_UNIVERSE_CHANGED, tmp_vec);
                event::EventBus::publish_event(event);
                auto pos = std::make_shared<FuturePosition>(order_book_id);
                positions_[order_book_id] = pos;
                return positions_[order_book_id];
            }
        }
    } /* trade */
} /* iqt */
