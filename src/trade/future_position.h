#ifndef _IQT_TRADE_FUTURE_POSITION_H_
#define _IQT_TRADE_FUTURE_POSITION_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <list>

//#include "env.h"
#include "const.hpp"
#include "model/tick.h"
#include "model/position.h"
#include "model/order.h"
#include "model/trade.h"
//#include "trade/abs_broker.h"

namespace iqt {
//    namespace model {
//        struct Trade;
//    }

    namespace trade {
        class Postions;

        class FuturePosition {
        public:
            FuturePosition(const std::string &order_book_id) : order_book_id_(order_book_id) {}

            void update_last_price();

            double apply_trade(std::shared_ptr<model::Trade> trade);
            double _close_holding(std::shared_ptr<model::Trade> trade);
            double _cal_realized_pnl(double cost_price, double trade_price, const trade::SIDE_TYPE &side, int consumed_quantity) {
                int multi_one = side == trade::SIDE_TYPE::BUY ? 1 : -1;
                return multi_one * (cost_price - trade_price) * consumed_quantity * contract_multiplier();
            }

            double buy_avg_holding_price() {
                double price = 0;
                if (buy_quantity() > 0) {
                    price = _buy_holding_cost() / buy_quantity() / contract_multiplier();
                }
                return price;
            }

            double sell_avg_holding_price() {
                double price = 0;
                if (sell_quantity() > 0) {
                    price = _sell_holding_cost() / sell_quantity() / contract_multiplier();
                }
                return price;
            }

            double buy_holding_pnl() {
                return (last_price_ - buy_avg_open_price_) * buy_quantity() * contract_multiplier();
            }
            double sell_holding_pnl() {
                return (sell_avg_open_price_ - last_price_) * buy_quantity() * contract_multiplier();
            }

            double holding_pnl() {
                return buy_holding_pnl() + sell_holding_pnl();
            }

            double buy_realized_pnl() {
                return buy_realized_pnl_;
            }
            double sell_realized_pnl() {
                return sell_realized_pnl_;
            }
            double realized_pnl() {
                return buy_realized_pnl() + sell_realized_pnl();
            }
            double buy_daily_pnl() {
                return buy_holding_pnl() + buy_realized_pnl();
            }
            double sell_daily_pnl() {
                return sell_holding_pnl() + sell_realized_pnl();
            }
            double daily_pnl() {
                return holding_pnl() + realized_pnl();
            }
            double buy_pnl() {
                return (last_price_ - buy_avg_open_price_) * buy_quantity() * contract_multiplier();
            }
            double sell_pnl() {
                return (sell_avg_open_price_ - last_price_) * sell_quantity() * contract_multiplier();
            }
            double pnl() {
                return buy_pnl() + sell_pnl();
            }
            //Quantity relative
            std::list<std::shared_ptr<model::Order>> &open_orders();

            int buy_open_order_quantity() {
                int sum = 0;
                for (auto &order : open_orders()) {
                    if (order->side == trade::SIDE_TYPE::BUY && order->position_effect == trade::POSITION_EFFECT_TYPE::OPEN) {
                        sum += order->quantity - order->filled_quantity;
                    }
                }
                return sum;
            }
            int sell_open_order_quantity() {
                int sum = 0;
                for (auto &order : open_orders()) {
                    if (order->side == trade::SIDE_TYPE::SELL && order->position_effect == trade::POSITION_EFFECT_TYPE::OPEN) {
                        sum += order->quantity - order->filled_quantity;
                    }
                }
                return sum;
            }
            int buy_close_order_quantity() {
                int sum = 0;
                for (auto &order : open_orders()) {
                    if (order->side == trade::SIDE_TYPE::BUY && (order->position_effect == trade::POSITION_EFFECT_TYPE::CLOSE_TODAY ||
                                                                order->position_effect == trade::POSITION_EFFECT_TYPE::CLOSE)) {
                        sum += order->quantity - order->filled_quantity;
                    }
                }
                return sum;
            }
            int sell_close_order_quantity() {
                int sum = 0;
                for (auto &order : open_orders()) {
                    if (order->side == trade::SIDE_TYPE::SELL && (order->position_effect == trade::POSITION_EFFECT_TYPE::CLOSE_TODAY ||
                                                                 order->position_effect == trade::POSITION_EFFECT_TYPE::CLOSE)) {
                        sum += order->quantity - order->filled_quantity;
                    }
                }
                return sum;
            }
            int buy_old_quantity() {
                int sum = 0;
                for (auto &pair : buy_old_holding_list_) {
                    sum += pair.second;
                }
                return sum;
            }
            int buy_today_quantity() {
                int sum = 0;
                for (auto &pair : buy_today_holding_list_) {
                    sum += pair.second;
                }
                return sum;
            }
            int sell_old_quantity() {
                int sum = 0;
                for (auto &pair : sell_old_holding_list_) {
                    sum += pair.second;
                }
                return sum;
            }
            int sell_today_quantity() {
                int sum = 0;
                for (auto &pair : sell_today_holding_list_) {
                    sum += pair.second;
                }
                return sum;
            }

            int buy_quantity() {
                return buy_old_quantity() + buy_today_quantity();
            }
            int sell_quantity() {
                return sell_old_quantity() + sell_today_quantity();
            }
            int closable_buy_quantity() {
                return buy_quantity() - sell_close_order_quantity();
            }
            int closable_sell_quantity() {
                return sell_quantity() - buy_close_order_quantity();
            }

            int contract_multiplier();

            double margin_rate();

            double market_value() {
                return (buy_quantity() - sell_quantity()) * last_price_ * contract_multiplier();
            }
            double buy_market_value() {
                return buy_quantity() * last_price_ * contract_multiplier();
            }
            double sell_market_value() {
                return sell_quantity() * last_price_ * contract_multiplier();
            }

            double _buy_holding_cost() {
                double sum = 0;
                for (auto &pair : buy_old_holding_list_) {
                    sum += pair.first * pair.second * contract_multiplier();
                }
                for (auto &pair : buy_today_holding_list_) {
                    sum += pair.first * pair.second * contract_multiplier();
                }
                return sum;
            }

            double _sell_holding_cost() {
                double sum = 0;
                for (auto &pair : sell_old_holding_list_) {
                    sum += pair.first * pair.second * contract_multiplier();
                }
                for (auto &pair : sell_today_holding_list_) {
                    sum += pair.first * pair.second * contract_multiplier();
                }
                return sum;
            }

            double buy_margin() {
                return _buy_holding_cost() * margin_rate();
            }

            double sell_margin() {
                return _sell_holding_cost() * margin_rate();
            }

            double margin() {
                //TODO: 需要添加单向大边相关的处理逻辑
                return buy_margin() + sell_margin();
            }

            void set_buy_avg_open_price(double price) {
                buy_avg_open_price_ = price;
            }
            void set_sell_avg_open_price(double price) {
                sell_avg_open_price_ = price;
            }

            double transaction_cost() {
                return buy_transaction_cost_ + sell_transaction_cost_;
            }
            void set_buy_transaction_cost(double cost) {
                buy_transaction_cost_ = cost;
            }
            void set_sell_transaction_cost(double cost) {
                sell_transaction_cost_ = cost;
            }
            void set_buy_realized_pnl(double pnl) {
                buy_realized_pnl_ = pnl;
            }
            void set_sell_realized_pnl(double pnl) {
                sell_realized_pnl_ = pnl;
            }
            void set_buy_old_holding_list(std::list<std::pair<double, int>> list) {
                buy_old_holding_list_ = std::move(list);
            };
            void set_sell_old_holding_list(std::list<std::pair<double, int>> list) {
                sell_old_holding_list_ = std::move(list);
            };
            void set_buy_today_holding_list(std::list<std::pair<double, int>> list) {
                buy_today_holding_list_ = std::move(list);
            };
            void set_sell_today_holding_list(std::list<std::pair<double, int>> list) {
                sell_today_holding_list_ = std::move(list);
            };

        private:
            std::string order_book_id_;
            double last_price_{0};
            double buy_avg_open_price_{0};
            double sell_avg_open_price_{0};
            double buy_transaction_cost_{0};
            double sell_transaction_cost_{0};
            double buy_realized_pnl_{0};
            double sell_realized_pnl_{0};
            std::list<std::pair<double, int>> buy_old_holding_list_;
            std::list<std::pair<double, int>> sell_old_holding_list_;
            std::list<std::pair<double, int>> buy_today_holding_list_;
            std::list<std::pair<double, int>> sell_today_holding_list_;
        };

        class Positions {
        private:
            std::unordered_map<std::string, std::shared_ptr<FuturePosition>> positions_;
        public:
            std::shared_ptr<FuturePosition> &get_or_create(const std::string &order_book_id);

            std::shared_ptr<FuturePosition> &operator[](const std::string& order_book_id) {
                return get_or_create(order_book_id);
            }
            auto begin() noexcept -> decltype(positions_.begin())
            { return positions_.begin(); }
            auto begin() const noexcept -> decltype(positions_.begin())
            { return positions_.begin(); }
            auto cbegin() const noexcept -> decltype(positions_.cbegin())
            { return positions_.cbegin(); }
            auto end() noexcept -> decltype(positions_.end())
            { return positions_.end(); }
            auto end() const noexcept -> decltype(positions_.end())
            { return positions_.begin(); }
            auto cend() const noexcept -> decltype(positions_.cend())
            { return positions_.cend(); }

/*            const std::unordered_map<std::string, std::shared_ptr<FuturePosition>> &get_map() const {
                return positions_;
            };*/
        };
    } /* trade */
} /* iqt */
#endif /* end of include guard: _IQT_TRADE_FUTURE_POSITION_H_ */
