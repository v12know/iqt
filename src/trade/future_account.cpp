#include "trade/future_account.h"

//#include <functional>

#include "global.h"
#include "trade/future_position.h"
#include "event/event.h"
#include "env.h"
#include "model/order.h"
#include "model/trade.h"

namespace iqt {
    namespace trade {

        FutureAccount::FutureAccount(double total_cash, std::shared_ptr<Positions> positions, bool register_event) :
                total_cash_(total_cash), positions_(positions) {
            if (register_event) {
                this->register_event();
            }
        }

        inline double _frozen_cash_of_order(std::shared_ptr<model::Order> order) {
            if (order->position_effect == trade::POSITION_EFFECT_TYPE::OPEN) {
                return global::margin_of(order->order_book_id, order->quantity - order->filled_quantity, order->price);
            }
            return 0;
        }

        inline double _frozen_cash_of_trade(std::shared_ptr<model::Trade> trade) {
            if (trade->position_effect == trade::POSITION_EFFECT_TYPE::OPEN) {
                return global::margin_of(trade->order_book_id, trade->last_quantity, trade->frozen_price);
            }
            return 0;
        }

        void FutureAccount::register_event() {
            auto _settlement = [&](void *) {
            };
            auto _on_order_pending_new = [&](std::shared_ptr<model::Order> order) {
                frozen_cash_ += _frozen_cash_of_order(order);
            };
            auto _on_order_creation_reject = [&](std::shared_ptr<model::Order> order) {
                frozen_cash_ -= _frozen_cash_of_order(order);
            };
            auto _on_order_unsolicited_update = [&](std::shared_ptr<model::Order> order) {
                frozen_cash_ -= _frozen_cash_of_order(order);
            };
            auto _on_trade = [&](model::TradeTuple& trade) {
                _apply_trade(std::get<0>(trade));
            };
            auto _on_bar = [&](void *) {
                for (auto item : *positions_) {
                    item.second->update_last_price();
                }
            };
            auto _on_tick = [&](void *) {
                for (auto item : *positions_) {
                    item.second->update_last_price();
                }
            };
            event::EventBus::add_listener<void *>(event::SETTLEMENT, {
                    _settlement
            });
            event::EventBus::add_listener<model::OrderPtr>(event::ORDER_PENDING_NEW, {
                    _on_order_pending_new
            });
            event::EventBus::add_listener<model::OrderPtr>(event::ORDER_CREATION_REJECT, {
                    _on_order_creation_reject
            });
            event::EventBus::add_listener<model::OrderPtr>(event::ORDER_CANCELLATION_PASS, {
                    _on_order_unsolicited_update
            });
            event::EventBus::add_listener<model::OrderPtr>(event::ORDER_UNSOLICITED_UPDATE, {
                    _on_order_unsolicited_update
            });
            event::EventBus::add_listener<model::TradeTuple &>(event::TRADE, {
                    _on_trade
            });
//            event::EventBus::add_listener(event::ORDER_UNSOLICITED_UPDATE, {std::bind(&FutureAccount::_on_order_unsolicited_update, this/*, std::placeholders::_1*/)});
            if (aggr_update_last_price_) {
                event::EventBus::add_listener<void *>(event::BAR, {
                        _on_bar
                });
                event::EventBus::add_listener<void *>(event::TICK, {
                        _on_tick
                });
            }
        }



        void FutureAccount::_apply_trade(std::shared_ptr<model::Trade> trade) {
            if (backward_trade_set_.find(trade->trade_id) != backward_trade_set_.end()) {
                return;
            }
            auto pos = positions_->get_or_create(trade->order_book_id);
            auto delta_cash = pos->apply_trade(trade);
            transaction_cost_ += trade->transaction_cost();
            total_cash_ -= trade->transaction_cost();
            total_cash_ += delta_cash;
            frozen_cash_ -= _frozen_cash_of_trade(trade);
            backward_trade_set_.emplace(trade->trade_id);
        }



    } /* trade */
} /* iqt */
