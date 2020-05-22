#include "trade/portfolio.h"

#include <chrono>
#include <cmath>

#include "event/event.h"
#include "env.h"

namespace iqt {
    namespace trade {
        using std::chrono::system_clock;

        Portfolio::Portfolio(const std::string &start_date, double static_unit_net_value, double units,
                             std::shared_ptr<FutureAccount> account, bool register_event) :
                start_date_(start_date), static_unit_net_value_(static_unit_net_value), units_(units), account_(account) {
            if (register_event) {
                this->register_event();
            }
        }
        void Portfolio::register_event() {
            auto _pre_before_trading = [&](void *) {
                static_unit_net_value_ = unit_net_value();
            };

            event::EventBus::add_listener<void *>(event::SETTLEMENT, {
                    _pre_before_trading
            });
        }

        double Portfolio::annualized_returns() {
            auto curr_date = base::strptimep(Env::instance()->trading_date, "%Y%m%d");
            auto start_date = base::strptimep(start_date_, "%Y-%m-%d");
            long diff_hours = std::chrono::duration_cast<std::chrono::hours>(curr_date.time_since_epoch()).count() - std::chrono::duration_cast<std::chrono::hours>(start_date.time_since_epoch()).count();
            long diff_days = diff_hours / 24;
            return pow(unit_net_value(), DAYS_CNT::DAYS_A_YEAR / static_cast<double>(diff_days + 1)) - 1;
        }

        double Portfolio::total_value() {
            return account_->total_value();
        }

        std::shared_ptr<Positions> Portfolio::positions() {
            return account_->positions();
        }

        double Portfolio::cash() {
            return account_->cash();
        }

        double Portfolio::transaction_cost() {
            return account_->transaction_cost();
        }

        double Portfolio::market_value() {
            return account_->market_value();
        }

        double Portfolio::frozen_cash() {
            return account_->frozen_cash();
        }
    } /* trade */
} /* iqt */
