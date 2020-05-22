#ifndef _IQT_TRADE_FUTURE_ACCOUNT_H_
#define _IQT_TRADE_FUTURE_ACCOUNT_H_

#include <string>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "trade/future_position.h"

namespace iqt {
    namespace model {
        struct Order;
        struct Trade;
    }
    namespace trade {

        class FutureAccount {
        public:
            FutureAccount(double total_cash, std::shared_ptr<Positions> positions, bool register_event = true);
            void register_event();
            double total_value() {
                return total_cash_ + margin() + holding_pnl();
            }
            double margin() {
                double sum = 0;
                for (auto &item : *positions_) {
                    sum += item.second->margin();
                }
                return sum;
            }
            double holding_pnl() {
                double sum = 0;
                for (auto &item : *positions_) {
                    sum += item.second->holding_pnl();
                }
                return sum;
            }
            void set_frozen_cash(double frozen_cash) {
                frozen_cash_ = frozen_cash;
            }
            double frozen_cash() {
                return frozen_cash_;
            }
            std::shared_ptr<Positions> positions() {
                return positions_;
            }
            double cash() {
                return total_cash_ - frozen_cash_;
            }
            double transaction_cost() {
                return transaction_cost_;
            }
            double market_value() {
                double sum = 0;
                for (auto &item : *positions_) {
                    sum += item.second->market_value();
                }
                return sum;
            }

            const std::unordered_set<std::string> &backward_trade_set() const {
                return backward_trade_set_;
            }

        private:
            void _apply_trade(std::shared_ptr<model::Trade> trade);


        private:
            double total_cash_;
            double transaction_cost_ = 0;
            double frozen_cash_ = 0;
            bool aggr_update_last_price_ = false;
            std::shared_ptr<Positions> positions_;
            std::unordered_set<std::string> backward_trade_set_;
        };
    } /* trade */
} /* iqt */
#endif /* end of include guard: _IQT_TRADE_FUTURE_ACCOUNT_H_ */
