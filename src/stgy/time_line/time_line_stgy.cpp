#include "time_line_stgy.h"

#include <pybind11/pybind11.h>
#include "event/event.h"
#include "base/log.h"
#include "base/redis.h"
#include "trade/api_future.h"


#include "time_line_stgy_helper.hpp"

namespace iqt {
    namespace stgy {

        namespace py = pybind11;
        using namespace base::redis;

        TimeLineStgy::TimeLineStgy(const TimeLineConfig *conf) :
                api_future_(new trade::ApiFuture()),
                helper_(new TimeLineStgyHelper(conf)),
                config_(conf) {
            base::LogFactory::create_logger(config_->stgy_id, config_->log_level);
        }

        TimeLineStgy::~TimeLineStgy() {
            delete api_future_;
        }

        bool TimeLineStgy::init(StgyContext *) {
            log_trace_g(config_->stgy_id, "TimeLineStgy::init");
            auto r = RedisFactory::get_redis();
            helper_->load_stgy(*r);
            auto rpt_key = helper_->get_rpt_key();
            api_future_->set_rpt_key(rpt_key);

            helper_->load_rpt(*r);

            helper_->load_long_txn(*r);

            helper_->load_short_txn(*r);

            event::EventBus::add_listener<OrderPtr>(event::ORDER_UNSOLICITED_UPDATE, {
                    [&](OrderPtr ord) {
                        on_order(ord);
                    }, rpt_key
            });
            event::EventBus::add_listener<TradeTuple &>(event::TRADE, {
                    [&](TradeTuple &trade) {
                        on_trade(trade);
                    }, rpt_key
            });


            event::EventBus::add_listener<void *>(event::POST_AFTER_TRADING, {
                    [&](void *) {
                        post_after_trading(get_stgy_context());
                    }
            });



            r->commit();

            return true;
        }


        void TimeLineStgy::post_open(cpp_redis::client &r, int , OpenGroupPtr open_group) {
            auto &txn = *(open_group->txn);

            auto &slot = *(txn.txn_slots[open_group->slot_index]);
            slot.open += open_group->filled_unit_quantity;
            slot.opening = 0;
            helper_->txn_service_->set_txn_slot(r, txn.txn_slots_key, open_group->slot_index, slot);

            txn.open_count += open_group->filled_unit_quantity;
            txn.opening_count = 0;
            helper_->txn_service_->update_txn(r, txn);
        }

        void TimeLineStgy::post_buy_open(cpp_redis::client &r, OrderPtr ord) {
            if (ord->side != trade::SIDE_TYPE::BUY) {
                return;
            }
//            log_trace_g(config_->stgy_id, "post_buy_open--order");

            helper_->order_service_->update_order(r, *ord);
            auto open_group = helper_->update_open_group(r, *ord);
            log_trace_g(config_->stgy_id, "post_buy_open open_group: {0}", open_group->to_string());
            switch (open_group->group_type) {
                case group::GROUP_TYPE_TYPE::NORMAL: {
                    helper_->long_txn_sm_->guard();
                    post_open(r, sign::POS, open_group);
                    r.commit();
                    break;
                }
                default:
                    ;
            }

        }
        void TimeLineStgy::post_sell_open(cpp_redis::client &r, OrderPtr ord) {
            if (ord->side != trade::SIDE_TYPE::SELL) {
                return;
            }
//            log_trace_g(config_->stgy_id, "post_sell_open--order");

            helper_->order_service_->update_order(r, *ord);
            auto open_group = helper_->update_open_group(r, *ord);
            log_trace_g(config_->stgy_id, "post_sell_open open_group: {0}", open_group->to_string());
            switch (open_group->group_type) {
                case group::GROUP_TYPE_TYPE::NORMAL: {
                    helper_->short_txn_sm_->guard();
                    post_open(r, sign::NEG, open_group);
                    r.commit();
                    break;
                }
                default:
                    ;
            }

        }

        void TimeLineStgy::post_close(cpp_redis::client &r, int , CloseGroupPtr close_group) {
            if (leg::FRAG_STATUS_TYPE::RCV_ONE == close_group->leg_items[0]->frag_status) {//只接收到一份order，等待另一份
                return;
            }
            auto &txn = *(close_group->txn);
            int filled_unit_quantity = close_group->filled_unit_quantity;
            if (filled_unit_quantity > 0) return;//只处理没有成功交易笔数的情况
            for (auto &open_group : close_group->open_groups) {
                open_group->closing_unit_quantity = 0;
                helper_->group_service_->update_open_group_quantity(r, *open_group);
            }
            txn.closing_count = 0;
            txn.open_count -= close_group->filled_unit_quantity;
            helper_->txn_service_->update_txn(r, txn);
            helper_->reset_txn_slots(r, txn.txn_slots_key, txn.txn_slots, close_group->filled_unit_quantity);


        }


        void TimeLineStgy::post_sell_close(cpp_redis::client &r, OrderPtr ord) {
            if (ord->side != trade::SIDE_TYPE::SELL) {
                return;
            }
//            log_trace_g(config_->stgy_id, "post_sell_close--order");
            helper_->order_service_->update_order(r, *ord);
            auto close_group = helper_->update_close_group(r, *ord);
            log_trace_g(config_->stgy_id, "post_sell_close close_group: {0}", close_group->to_string());
            switch (close_group->group_type) {
                case group::GROUP_TYPE_TYPE::NORMAL: {
                    helper_->long_txn_sm_->guard();
                    post_close(r, sign::POS, close_group);
                    r.commit();
                    break;
                }
                default:
                    ;
            }


        }
        void TimeLineStgy::post_buy_close(cpp_redis::client &r, OrderPtr ord) {
            if (ord->side != trade::SIDE_TYPE::BUY) {
                return;
            }
//            log_trace_g(config_->stgy_id, "post_buy_close--order");

            helper_->order_service_->update_order(r, *ord);
            auto close_group = helper_->update_close_group(r, *ord);
            log_trace_g(config_->stgy_id, "post_buy_close close_group: {0}", close_group->to_string());

            switch (close_group->group_type) {
                case group::GROUP_TYPE_TYPE::NORMAL: {
                    helper_->short_txn_sm_->guard();
                    post_close(r, sign::NEG, close_group);
                    r.commit();
                    break;
                }
                default:
                    ;
            }


        }
        void TimeLineStgy::post_open_after_trade(cpp_redis::client &r, int sign, OpenGroupPtr open_group) {
            auto &txn = *(open_group->txn);

            helper_->add_txn_cost(r, sign, txn.txn_costs_key, txn.txn_costs, open_group);

            helper_->update_open_pnl_cost(r, sign, *open_group);
        }

        void TimeLineStgy::post_buy_open_after_trade(cpp_redis::client &r, TradeTuple &trade) {
            if (std::get<0>(trade)->side != trade::SIDE_TYPE::BUY) {
                return;
            }
//            log_trace_g(config_->stgy_id, "post_buy_open--trade");
            helper_->order_service_->update_order_after_trade(r, trade);

            auto open_group = helper_->update_open_group_after_trade(r, trade);
            log_trace_g(config_->stgy_id, "post_buy_open_after_trade open_group: {0}", open_group->to_string());
            switch (open_group->group_type) {
                case group::GROUP_TYPE_TYPE::NORMAL: {
                    helper_->long_txn_sm_->guard();
                    post_open_after_trade(r, sign::POS, open_group);
                    r.commit();
                    break;
                }
                default:
                    ;
            }

        }
        void TimeLineStgy::post_sell_open_after_trade(cpp_redis::client &r,
                                                        TradeTuple &trade) {
            if (std::get<0>(trade)->side != trade::SIDE_TYPE::SELL) {
                return;
            }
//            log_trace_g(config_->stgy_id, "post_sell_open--trade");
            helper_->order_service_->update_order_after_trade(r, trade);

            auto open_group = helper_->update_open_group_after_trade(r, trade);
            log_trace_g(config_->stgy_id, "post_sell_open_after_trade open_group: {0}", open_group->to_string());
            switch (open_group->group_type) {
                case group::GROUP_TYPE_TYPE::NORMAL: {
                    helper_->short_txn_sm_->guard();
                    post_open_after_trade(r, sign::NEG, open_group);
                    r.commit();
                    break;
                }
                default:
                    ;
            }

        }

        void TimeLineStgy::post_close_after_trade(cpp_redis::client &r, int sign, CloseGroupPtr close_group) {
            if (leg::FRAG_STATUS_TYPE::RCV_ONE == close_group->leg_items[0]->frag_status) {//只接收到一份order，等待另一份
                return;
            }
            auto &txn = *(close_group->txn);
            int filled_unit_quantity = close_group->filled_unit_quantity;
            for (auto &open_group : close_group->open_groups) {
                if (filled_unit_quantity >= open_group->closing_unit_quantity) {
                    close_group->realized_pnl += open_group->closing_unit_quantity * (close_group->avg_cost - open_group->avg_cost) * sign * helper_->leg0_->contract_multiplier;
                    open_group->close_unit_quantity += open_group->closing_unit_quantity;
                    filled_unit_quantity -= open_group->closing_unit_quantity;
                } else {
                    close_group->realized_pnl += filled_unit_quantity * (close_group->avg_cost - open_group->avg_cost) * sign * helper_->leg0_->contract_multiplier;
                    open_group->close_unit_quantity += filled_unit_quantity;
                    filled_unit_quantity = 0;
                }

                open_group->closing_unit_quantity = 0;
                helper_->group_service_->update_open_group_quantity(r, *open_group);

                if (open_group->filled_unit_quantity == open_group->close_unit_quantity) {//open_group已经没有合约数，应从txn_costs列表中删除
                    helper_->remove_txn_cost(r, txn.txn_costs_key, txn.txn_costs, open_group);
                }
            }
            helper_->update_close_pnl_cost(r, sign, *close_group);

            txn.closing_count = 0;
            txn.open_count -= close_group->filled_unit_quantity;
            helper_->reset_txn_slots(r, txn.txn_slots_key, txn.txn_slots, close_group->filled_unit_quantity);

//            auto &slot = *(txn.txn_slots[0]);
//            switch (txn.status) {
//                case txn::STATUS_TYPE::STOP: {
//                    if (0 == slot.open) {
//                        txn.status = txn::STATUS_TYPE::EXIT;
//                    }
//                    break;
//                }
//                default:
//                    ;
//            }
            helper_->txn_service_->update_txn(r, txn);
        }


        void TimeLineStgy::post_sell_close_after_trade(cpp_redis::client &r, TradeTuple &trade) {
            if (std::get<0>(trade)->side != trade::SIDE_TYPE::SELL) {
                return;
            }
//            log_trace_g(config_->stgy_id, "post_sell_close--trade");
            helper_->order_service_->update_order_after_trade(r, trade);
            auto close_group = helper_->update_close_group_after_trade(r, trade);
            log_trace_g(config_->stgy_id, "post_sell_close_after_trade close_group: {0}", close_group->to_string());

            switch (close_group->group_type) {
                case group::GROUP_TYPE_TYPE::NORMAL: {
                    helper_->long_txn_sm_->guard();
                    post_close_after_trade(r, sign::POS, close_group);
                    r.commit();
                    break;
                }
                default:
                    ;
            }

        }
        void TimeLineStgy::post_buy_close_after_trade(cpp_redis::client &r,
                                                        TradeTuple &trade) {
            if (std::get<0>(trade)->side != trade::SIDE_TYPE::BUY) {
                return;
            }
//            log_trace_g(config_->stgy_id, "post_buy_close--trade");
            helper_->order_service_->update_order_after_trade(r, trade);
            auto close_group = helper_->update_close_group_after_trade(r, trade);
            log_trace_g(config_->stgy_id, "post_buy_close_after_trade close_group: {0}", close_group->to_string());

            switch (close_group->group_type) {
                case group::GROUP_TYPE_TYPE::NORMAL: {
                    helper_->short_txn_sm_->guard();
                    post_close_after_trade(r, sign::NEG, close_group);
                    r.commit();
                    break;
                }
                default:
                    ;
            }

        }

        void TimeLineStgy::on_order(std::shared_ptr<Order> ord) {
            auto r = RedisFactory::get_redis();
            switch (ord->position_effect) {
                case trade::POSITION_EFFECT_TYPE::OPEN: {
                    post_buy_open(*r, ord);
                    post_sell_open(*r, ord);
                    break;
                }
                case trade::POSITION_EFFECT_TYPE::CLOSE:
                case trade::POSITION_EFFECT_TYPE::CLOSE_TODAY: {
                    post_sell_close(*r, ord);
                    post_buy_close(*r, ord);
                    break;
                }
//                default:
//                    ;
            }
        }

        void TimeLineStgy::on_trade(TradeTuple &trade) {
            auto r = RedisFactory::get_redis();
            switch (std::get<1>(trade)->position_effect) {
                case trade::POSITION_EFFECT_TYPE::OPEN: {
                    post_buy_open_after_trade(*r, trade);
                    post_sell_open_after_trade(*r, trade);
                    break;
                }
                case trade::POSITION_EFFECT_TYPE::CLOSE:
                case trade::POSITION_EFFECT_TYPE::CLOSE_TODAY: {
                    post_sell_close_after_trade(*r, trade);
                    post_buy_close_after_trade(*r, trade);
                    break;
                }
//                default:
//                    ;
            }
        }
        void TimeLineStgy::open(cpp_redis::client &r, std::shared_ptr<Tick> tick, int sign, TxnPtr txn, order_func_t order_func) {
            switch (txn->status) {
                case txn::STATUS_TYPE::RUNNING: {
                    auto slot_index = 0;
                    auto &cur_slot = txn->txn_slots[slot_index];
                    if (txn->opening_count > 0) return;//有正在开仓的则直接退出
                    auto price = sign::POS == sign ? tick->a1 : tick->b1;
                    auto volume = sign::POS == sign ? tick->a1_v : tick->b1_v;
                    int can_open = cur_slot->capacity - cur_slot->open;
                    can_open = std::min(can_open, volume);
                    if (can_open > 0) {
                        auto now_time = base::strftimep(tick->datetime, "%H%M");
                        double up_multiply = 3;
                        double down_multiply = 1;
//                        double up_open = 1.5;
                        if ((now_time >= "2100" && now_time < "2115") || (now_time >= "0900" && now_time < "0915")) {
                            up_multiply = 3.5;
                            down_multiply = 1.5;
                        }
                        if ( sign * tick->settlement + config_->spread * down_multiply <= sign * tick->last
                                && sign * tick->settlement + config_->spread * up_multiply >= sign * tick->last//in spread's [1,2] section, can open
                                && tick->open * sign < tick->last * sign) {
//                                && sign * tick->settlement + config_->spread * up_open > sign * tick->open) {
                            auto orders = order_func(helper_->leg0_->order_book_id, can_open, price);
                            if (!orders.empty()) {
                                auto open_group = std::make_shared<OpenGroup>();
                                open_group->slot_index = slot_index;
                                open_group->group_type = group::GROUP_TYPE_TYPE::NORMAL;
                                helper_->add_open_group(r, txn, open_group, orders[0]);
                                helper_->order_service_->add_order(r, orders[0]);

                                txn->opening_count += can_open;
                                helper_->txn_service_->update_txn(r, *txn);
                                cur_slot->opening = can_open;
                                helper_->txn_service_->set_txn_slot(r, txn->txn_slots_key, slot_index, *cur_slot);
                                log_trace_g(config_->stgy_id, open_group->to_string());
                                log_trace_g(config_->stgy_id, tick->to_string());
                                log_trace_g(config_->stgy_id, txn->to_string());
                            }
                        }
                    }
                    break;
                }
                default:;
            }
        }

        void TimeLineStgy::buy_open(cpp_redis::client &r, std::shared_ptr<Tick> tick) {
//            log_trace_g(config_->stgy_id, "buy_open");
            {
                helper_->long_txn_sm_->guard();
                open(r, tick, sign::POS, helper_->long_txn_,
                     [&](const std::string &order_book_id, int amount, double price) {
                         return api_future_->buy_open(order_book_id, amount, price);
                     });
                r.commit();
            }
        }
        void TimeLineStgy::sell_open(cpp_redis::client &r, std::shared_ptr<Tick> tick) {
//            log_trace_g(config_->stgy_id, "sell_open");
            {
                helper_->short_txn_sm_->guard();
                open(r, tick, sign::NEG, helper_->short_txn_,
                     [&](const std::string &order_book_id, int amount, double price) {
                         return api_future_->sell_open(order_book_id, amount, price);
                     });
                r.commit();
            }
        }

        void TimeLineStgy::close(cpp_redis::client &r, std::shared_ptr<Tick> tick, int sign, TxnPtr txn, order_func_t order_func) {
            bool force_close = false;
            switch (txn->status) {
                case txn::STATUS_TYPE::STOP://nearly close time, need close all positions and exit
                case txn::STATUS_TYPE::BLOCK://must clear yesterday positions
                    force_close = true;
                case txn::STATUS_TYPE::RUNNING: {
                    if (txn->closing_count > 0) return;//有正在平仓的group，直接返回

                    auto price = sign::POS == sign ? tick->b1 : tick->a1;
                    auto volume = sign::POS == sign ? tick->b1_v : tick->a1_v;
                    auto limit = sign::POS == sign ? tick->limit_up : tick->limit_down;
                    if (abs(price - limit) <= 1e-6) {//today's max price, stop profit and exit

                        txn->status = txn::STATUS_TYPE::STOP;
                        force_close = true;
                    }

                    auto close_group = std::make_shared<CloseGroup>();
                    for (auto &txn_cost : txn->txn_costs) {
                        auto &open_group = txn_cost->open_group;
                        if (force_close
                            || sign * tick->settlement > price * sign//stop profit
                            || price * sign + 1.5 * config_->spread <= txn_cost->avg_cost * sign) {//stop loss
                            if (helper_->scan_can_close(r, open_group, close_group, volume)) {
                                break;
                            }
                        } else {
                            break;
                        }
                    }
                    if (close_group->unit_quantity > 0) {
                        auto orders = order_func(helper_->leg0_->order_book_id, close_group->unit_quantity, price);
                        if (!orders.empty()) {
                            close_group->avg_cost = price;
                            close_group->group_type = group::GROUP_TYPE_TYPE::NORMAL;
                            helper_->add_close_group(r, txn, close_group, orders);
                            txn->closing_count = close_group->unit_quantity;
                            helper_->txn_service_->update_txn(r, *txn);
                            log_debug_g(config_->stgy_id, tick->to_string());
                            log_debug_g(config_->stgy_id, close_group->to_string());
                            log_debug_g(config_->stgy_id, txn->to_string());
                        } else {//not enough positions to close, must cancel closing
                            for (auto &open_group : close_group->open_groups) {
                                open_group->closing_unit_quantity = 0;
                                helper_->group_service_->update_open_group_quantity(r, *open_group);
                            }
                        }
                    }
                    break;
                }
                default:
                    ;
            }

        }
        void TimeLineStgy::sell_close(cpp_redis::client &r, std::shared_ptr<Tick> tick) {
//            log_trace_g(config_->stgy_id, "sell_close");
            {
                helper_->long_txn_sm_->guard();
                close(r, tick, sign::POS, helper_->long_txn_,
                      [&](const std::string &order_book_id, int amount, double price) {
                          return api_future_->sell_close(order_book_id, amount, price, config_->close_today);
                      });
                r.commit();
            }
        }
        void TimeLineStgy::buy_close(cpp_redis::client &r, std::shared_ptr<Tick> tick) {
//            log_trace_g(config_->stgy_id, "buy_close");
            {
                helper_->short_txn_sm_->guard();
                close(r, tick, sign::NEG, helper_->short_txn_,
                      [&](const std::string &order_book_id, int amount, double price) {
                          return api_future_->buy_close(order_book_id, amount, price, config_->close_today);
                      });
                r.commit();
            }
        }

        void TimeLineStgy::handle_tick(StgyContext *, std::shared_ptr<Tick> tick) {
//            log_trace_g(config_->stgy_id, "TimeLineStgy::handle_tick");

            tick->fix_settlement();

            auto r = RedisFactory::get_redis();
//            auto tick1 = Env::instance()->snapshot[helper_->leg0_];

            handle_txn_status(*r, tick, sign::POS, *(helper_->long_txn_));
            handle_txn_status(*r, tick, sign::NEG, *(helper_->short_txn_));

            buy_open(*r, tick);
            sell_close(*r, tick);
            sell_open(*r, tick);
            buy_close(*r, tick);


//            log_trace_g(config_->stgy_id, "tick={0}", tick->to_string());
/*            log_trace_g(config_->stgy_id, "stgy_id={0}, order_book_id={1}", config_->stgy_id, tick->order_book_id);
            auto positions = context->portfolio()->positions();
            (*positions)[tick->order_book_id];
            auto pos = (*positions)[tick->order_book_id];
            int quan = pos->sell_quantity();
            log_debug_g(config_->stgy_id, "pos.sell_quantity={0}, buy_quantity={1}", pos->sell_quantity(), pos->buy_quantity());
            static int count = 0;
            if (count == 0) {
                api_future_->sell_open(tick->order_book_id, 1, tick->b1);
                ++count;
                return;
            }
            if (count == 1) {
                api_future_->buy_close(tick->order_book_id, 1, tick->a1);
                ++count;
                return;
            }*/
        }

        void TimeLineStgy::handle_txn_status(cpp_redis::client &r, std::shared_ptr<Tick> tick, int sign, Txn &txn) {
//            auto price = sign::POS == sign ? tick->a1 : tick->b1;
            switch (txn.status) {
                case txn::STATUS_TYPE::ONLY_CLOSE:
                case txn::STATUS_TYPE::RUNNING: {
                    auto now_datetime = base::strftimep(tick->datetime, "%Y%m%d%H%M");
                    auto now_date = now_datetime.substr(0, 8);
                    auto now_time = now_datetime.substr(8);
                    if (now_date == Env::instance()->trading_date && now_time >= "1458") {
                        txn.status = txn::STATUS_TYPE::STOP;
                        helper_->txn_service_->update_txn(r, txn);
                    }
                    if (now_time >= "1445") {
                        txn.status = txn::STATUS_TYPE::ONLY_CLOSE;
                        helper_->txn_service_->update_txn(r, txn);
                    }
                    break;
                }
                case txn::STATUS_TYPE::STOP: {
                    if (0 == txn.txn_slots[0]->open) {
                        txn.status = txn::STATUS_TYPE::EXIT;
                        helper_->txn_service_->update_txn(r, txn);
                    }
                    break;
                }
                case txn::STATUS_TYPE::BLOCK: {
                    if (0 == txn.txn_slots[0]->open) {
                        txn.status = txn::STATUS_TYPE::RUNNING;
                        helper_->txn_service_->update_txn(r, txn);
                    }
                    break;

                }
                default:;
            }
        }

        void TimeLineStgy::before_trading(StgyContext *) {
            log_warn_g(config_->stgy_id, "TimeLineStgy::before_trading");
            auto r = RedisFactory::get_redis();
            if (helper_->long_txn_->status == txn::STATUS_TYPE::STOP) {//must clear yesterday positions
                helper_->long_txn_->status = txn::STATUS_TYPE::BLOCK;
            } else {
                helper_->long_txn_->status = txn::STATUS_TYPE::RUNNING;
            }
            helper_->txn_service_->update_txn(*r, *helper_->long_txn_);


            if (helper_->short_txn_->status == txn::STATUS_TYPE::STOP) {//must clear yesterday positions
                helper_->short_txn_->status = txn::STATUS_TYPE::BLOCK;
            } else {
                helper_->short_txn_->status = txn::STATUS_TYPE::RUNNING;
            }
            helper_->txn_service_->update_txn(*r, *helper_->short_txn_);
            r->commit();
        }

        void TimeLineStgy::after_trading(StgyContext *) {
            log_warn_g(config_->stgy_id, "TimeLineStgy::after_trading");
        }

        void TimeLineStgy::post_after_trading(StgyContext *) {
            log_warn_g(config_->stgy_id, "TimeLineStgy::post_after_trading");
            static bool flag = false;
            if (!flag) {
                flag = true;
            }
        }

    } /* stgy  */
} /* iqt */ 
