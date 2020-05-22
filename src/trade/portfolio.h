#ifndef _IQT_TRADE_PORTFOLIO_H_
#define _IQT_TRADE_PORTFOLIO_H_

#include <memory>
#include <string>
#include "trade/future_position.h"
#include "trade/future_account.h"


namespace iqt {
    namespace trade {
/*        class FutureAccount;
        class Positions;*/

        class Portfolio {
        public:
            Portfolio(const std::string &start_date, double static_unit_net_value, double units, std::shared_ptr<FutureAccount> account, bool register_event = true);
            void register_event();
            void _pre_before_trading() {
                static_unit_net_value_ = unit_net_value();
            }
            double units() {
                return units_;
            }
            double unit_net_value() {
                return total_value() / units_;
            }
            double static_unit_net_value() {
                return static_unit_net_value_;
            }
            double daily_pnl() {
                return total_value() - static_unit_net_value_ * units_;
            }
            double daily_returns() {
                double ret = 0;
                if (static_unit_net_value_ > 0) {
                    ret = unit_net_value() / static_unit_net_value_ - 1;
                }
                return ret;
            }
            double annualized_returns();

            double total_value();

            double portfolio_value() {
                return total_value();
            }

            std::shared_ptr<Positions> positions();

            double cash();

            double transaction_cost();

            double market_value();

            double pnl() {
                return (unit_net_value() - 1) * units();
            }

            double start_cash() {
                return units();
            }

            double frozen_cash();

            std::shared_ptr<FutureAccount> account() {
                return account_;
            }
        private:
            std::string start_date_;
            double static_unit_net_value_;
            double units_;
            std::shared_ptr<FutureAccount> account_;

        };

    } /* model  */

} /* iqt */
#endif /* end of include guard: _IQT_TRADE_PORTFOLIO_H_ */
