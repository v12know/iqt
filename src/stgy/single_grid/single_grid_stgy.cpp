#include "single_grid_stgy.h"

#include <pybind11/pybind11.h>
#include "event/event.h"
#include "base/log.h"
#include "base/redis.h"
#include "trade/api_future.h"


#include "single_grid_stgy_helper.hpp"

namespace iqt {
    namespace stgy {

        namespace py = pybind11;
        using namespace base::redis;

        SingleGridStgy::SingleGridStgy(const SingleGridConfig *conf) :
                api_future_(new trade::ApiFuture()),
                helper_(new SingleGridStgyHelper(conf)),
                config_(conf) {
            base::LogFactory::create_logger(config_->stgy_id, config_->log_level);
        }

        SingleGridStgy::~SingleGridStgy() {
            delete api_future_;
        }

        bool SingleGridStgy::init(StgyContext *) {
            log_trace_g(config_->stgy_id, "SingleGridStgy::init");
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


            {
                py::gil_scoped_acquire acquire;
                py::object get_signal_func = py::module::import("pyiqt.stgy.single_grid.single_grid_stgy").attr("get_signal");

                py::tuple signal_tuple = get_signal_func(helper_->leg0_->order_book_id);

                auto ma5_ols = signal_tuple[0].cast<double>();
                auto ma20_ols = signal_tuple[1].cast<double>();
                auto ma20_dis = signal_tuple[2].cast<double>();
                auto atr = signal_tuple[3].cast<double>();
                helper_->update_signal(ma5_ols, ma20_ols, ma20_dis, atr);

                py::object register_stgy_func = py::module::import("pyiqt.stgy.single_grid.single_grid_stgy").attr("register_stgy");

                register_stgy_func(helper_->leg0_->order_book_id, this);
            }

            r->commit();

            return true;
        }


        void SingleGridStgy::update_signal(double ma5_ols, double ma20_ols, double ma20_dis, double atr) {
            py::gil_scoped_release release;
            helper_->update_signal(ma5_ols, ma20_ols, ma20_dis, atr);
        }

        void SingleGridStgy::post_open(cpp_redis::client &r, int , OpenGroupPtr open_group) {
            auto &txn = *(open_group->txn);
            txn.open_count += open_group->filled_unit_quantity;
            txn.opening_count = 0;
            helper_->txn_service_->update_txn(r, txn);
            auto &slot = *(txn.txn_slots[open_group->slot_index]);
            slot.open += open_group->filled_unit_quantity;
            slot.opening = 0;
            helper_->txn_service_->set_txn_slot(r, txn.txn_slots_key, open_group->slot_index, slot);

        }
        void SingleGridStgy::post_lock(cpp_redis::client &r, int , OpenGroupPtr open_group) {
            auto &txn = *(open_group->txn);
            txn.lock_count += open_group->filled_unit_quantity;
            txn.opening_count = 0;
            helper_->txn_service_->update_txn_lock(r, txn);
//            helper_->txn_service_->update_txn_count(r, txn);
        }

        void SingleGridStgy::post_buy_open(cpp_redis::client &r, OrderPtr ord) {
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
                case group::GROUP_TYPE_TYPE::LOCK: {
                    helper_->short_txn_sm_->guard();
                    post_lock(r, sign::POS, open_group);
                    r.commit();
                    break;
                }
//                default:
//                    ;
            }

        }
        void SingleGridStgy::post_sell_open(cpp_redis::client &r, OrderPtr ord) {
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
                case group::GROUP_TYPE_TYPE::LOCK: {
                    helper_->long_txn_sm_->guard();
                    post_lock(r, sign::NEG, open_group);
                    r.commit();
                    break;
                }
//                default:
//                    ;
            }

        }

        void SingleGridStgy::post_close(cpp_redis::client &r, int , CloseGroupPtr close_group) {
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

        void SingleGridStgy::post_unlock(cpp_redis::client &r, int , CloseGroupPtr close_group) {
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
            txn.lock_count -= close_group->filled_unit_quantity;
            helper_->txn_service_->update_txn_lock(r, txn);

        }

        void SingleGridStgy::post_sell_close(cpp_redis::client &r, OrderPtr ord) {
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
                case group::GROUP_TYPE_TYPE::LOCK: {
                    helper_->short_txn_sm_->guard();
                    post_unlock(r, sign::POS, close_group);
                    r.commit();
                    break;
                }
//                default:
//                    ;
            }


        }
        void SingleGridStgy::post_buy_close(cpp_redis::client &r, OrderPtr ord) {
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
                case group::GROUP_TYPE_TYPE::LOCK: {
                    helper_->long_txn_sm_->guard();
                    post_unlock(r, sign::NEG, close_group);
                    r.commit();
                    break;
                }
//                default:
//                    ;
            }


        }
        void SingleGridStgy::post_open_after_trade(cpp_redis::client &r, int sign, OpenGroupPtr open_group) {
            auto &txn = *(open_group->txn);

            helper_->add_txn_cost(r, sign, txn.txn_costs_key, txn.txn_costs, open_group);

            switch (txn.status) {
                case txn::STATUS_TYPE::READY: {
                    txn.base_price = open_group->avg_cost;
                    txn.status = txn::STATUS_TYPE::RUNNING;
                    helper_->txn_service_->update_txn_lock(r, txn);
                    break;
                }
//                case txn::STATUS_TYPE::SEMI_LOCK:
                case txn::STATUS_TYPE::RUNNING: {
                    if (txn.base_price * sign > open_group->avg_cost * sign) {
                        txn.base_price = open_group->avg_cost;
                        helper_->txn_service_->update_txn_lock(r, txn);
                    }
                }
                default:
                    ;
            }
            helper_->update_open_pnl_cost(r, sign, *open_group);
        }
        void SingleGridStgy::post_lock_after_trade(cpp_redis::client &r, int sign, OpenGroupPtr open_group) {
            auto &txn = *(open_group->txn);


            helper_->add_txn_cost(r, sign, txn.txn_locks_key, txn.txn_locks, open_group);
            txn.lock_price = (txn.lock_price * (txn.lock_count - open_group->filled_unit_quantity)
                               + open_group->avg_cost * open_group->filled_unit_quantity) / txn.lock_count;
            txn.lock_date = Env::instance()->trading_date;

            if (txn.lock_count >= txn.lockable_count) {
                assert(txn::STATUS_TYPE::LOCKING == txn.status);
//                txn.status = txn::STATUS_TYPE::SEMI_LOCK;
                txn.status = txn::STATUS_TYPE::LOCK;
            }
            helper_->txn_service_->update_txn_lock(r, txn);
            helper_->update_open_pnl_cost(r, -sign, *open_group);
        }

        void SingleGridStgy::post_buy_open_after_trade(cpp_redis::client &r, TradeTuple &trade) {
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
                case group::GROUP_TYPE_TYPE::LOCK: {
                    helper_->short_txn_sm_->guard();
                    post_lock_after_trade(r, sign::POS, open_group);
                    r.commit();
                    break;
                }
//                default:
//                    ;
            }

        }
        void SingleGridStgy::post_sell_open_after_trade(cpp_redis::client &r,
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
                case group::GROUP_TYPE_TYPE::LOCK: {
                    helper_->long_txn_sm_->guard();
                    post_lock_after_trade(r, sign::NEG, open_group);
                    r.commit();
                    break;
                }
//                default:
//                    ;
            }

        }

        void SingleGridStgy::post_close_after_trade(cpp_redis::client &r, int sign, CloseGroupPtr close_group) {
            if (leg::FRAG_STATUS_TYPE::RCV_ONE == close_group->leg_items[0]->frag_status) {//只接收到一份order，等待另一份
                return;
            }
            auto &txn = *(close_group->txn);
            bool check_flag = false;
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
                    check_flag = true;
                    helper_->remove_txn_cost(r, txn.txn_costs_key, txn.txn_costs, open_group);
                }
            }
            helper_->update_close_pnl_cost(r, sign, *close_group);
            txn.closing_count = 0;
            txn.open_count -= close_group->filled_unit_quantity;
            helper_->txn_service_->update_txn(r, txn);
            helper_->reset_txn_slots(r, txn.txn_slots_key, txn.txn_slots, close_group->filled_unit_quantity);

            if (check_flag && !txn.txn_costs.empty()) {//如果open_groups列表有数据被删除，则需要检查新的基准价格
                if (txn.txn_costs.front()->avg_cost * sign > txn.base_price * sign) {
                    txn.base_price = txn.txn_costs.front()->avg_cost;
                    helper_->txn_service_->update_txn_lock(r, txn);
                }
            }
        }
        void SingleGridStgy::post_unlock_after_trade(cpp_redis::client &r, int sign, CloseGroupPtr close_group) {

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
                    helper_->remove_txn_cost(r, txn.txn_locks_key, txn.txn_locks, open_group);
                }
            }
            helper_->update_close_pnl_cost(r, -sign, *close_group);

            txn.unlock_price = (txn.unlock_price * (txn.lockable_count - txn.lock_count)
                     + close_group->avg_cost * close_group->filled_unit_quantity) / (txn.lockable_count - txn.lock_count + close_group->filled_unit_quantity);
            txn.closing_count = 0;
            txn.lock_count -= close_group->filled_unit_quantity;

            if (0 == txn.lock_count) {
                assert(txn.txn_locks.empty());
                double diff_price = (txn.unlock_price - txn.lock_price) * txn.lockable_count / txn.open_count;
                for (auto &txn_cost : txn.txn_costs) {
                    txn_cost->avg_cost = txn_cost->avg_cost + diff_price;
                }
                helper_->txn_service_->update_txn_costs(r, txn.txn_costs_key, txn.txn_costs);
                txn.status = txn::STATUS_TYPE::RUNNING;


//                helper_->txn_service_->update_txn1(r, txn);

                txn.base_price = txn.txn_costs.front()->avg_cost;//更新base_price
            }

            helper_->txn_service_->update_txn_lock(r, txn);
        }


        void SingleGridStgy::post_sell_close_after_trade(cpp_redis::client &r, TradeTuple &trade) {
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
                case group::GROUP_TYPE_TYPE::LOCK: {
                    helper_->short_txn_sm_->guard();
                    post_unlock_after_trade(r, sign::POS, close_group);
                    r.commit();
                    break;
                }
//                default:
//                    ;
            }

        }
        void SingleGridStgy::post_buy_close_after_trade(cpp_redis::client &r,
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
                case group::GROUP_TYPE_TYPE::LOCK: {
                    helper_->long_txn_sm_->guard();
                    post_unlock_after_trade(r, sign::NEG, close_group);
                    r.commit();
                    break;
                }
//                default:
//                    ;
            }

        }

        void SingleGridStgy::on_order(std::shared_ptr<Order> ord) {
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

        void SingleGridStgy::on_trade(TradeTuple &trade) {
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
        void SingleGridStgy::lock(cpp_redis::client &r, double price, int volume, int sign, TxnPtr txn, order_func_t order_func) {
            switch (txn->status) {
                case txn::STATUS_TYPE::LOCKING: {
                    auto vol_uplimit = static_cast<int>(volume * config_->vol_use_rate);
                    if (vol_uplimit <= 0) {
                        return;
                    }
                    if (txn->opening_count > 0) {//有正在开仓的则直接退出
                        return;
                    }
                    int can_open = txn->lockable_count - txn->lock_count;
                    if (can_open <= 0) {
                        return;
                    }
                    log_trace_g(config_->stgy_id, "vol_uplimit={0}, can_open={1}, base_price={2}, price={3}, sign={4}",
                                vol_uplimit, can_open, txn->base_price, price, sign);
                    can_open = std::min(can_open, vol_uplimit);
                    auto orders = order_func(helper_->leg0_->order_book_id, can_open, price);
                    if (!orders.empty()) {

                        auto open_group = std::make_shared<OpenGroup>();
                        open_group->group_type = group::GROUP_TYPE_TYPE::LOCK;
                        helper_->add_open_group(r, txn, open_group, orders[0]);
                        log_trace_g(config_->stgy_id, open_group->to_string());
                        helper_->order_service_->add_order(r, orders[0]);

                        txn->opening_count += can_open;
                        helper_->txn_service_->update_txn_lock(r, *txn);
                    }
                    break;
                }
                default:
                    ;
            }
        }
        void SingleGridStgy::open(cpp_redis::client &r, double price, int volume, int sign, TxnPtr txn, order_func_t order_func) {
            switch (txn->status) {
                case txn::STATUS_TYPE::READY:
//                case txn::STATUS_TYPE::SEMI_LOCK:
                case txn::STATUS_TYPE::RUNNING: {
                    if (txn::STATUS_TYPE::READY == txn->status) {
                        txn->base_price = price;
                    }
/*                    if (txn::STATUS_TYPE::SEMI_LOCK == txn->status && txn->lock_count <= txn->open_count) {
                        return;
                    }*/
                    if (txn->base_price * sign >= price * sign) {
                        auto vol_uplimit = static_cast<int>(volume * config_->vol_use_rate);
                        if (vol_uplimit <= 0) {
                            return;
                        }
                        if (txn->opening_count > 0) {//有正在开仓的则直接退出
                            return;
                        }
                        int can_open = 0;
                        double open_spread = 0;
                        auto slots_size = static_cast<int>(txn->txn_slots.size());
                        int i = 0;
                        do {
                            for (; i < slots_size; ++i) {
                                int remain = txn->txn_slots[i]->capacity - txn->txn_slots[i]->open;
                                if (remain > 0) {
                                    can_open = remain;
                                    if (can_open == txn->txn_slots[i]->capacity && txn::STATUS_TYPE::READY != txn->status) {
                                        open_spread = config_->open_spread;
                                    }
                                    break;
                                }
                            }
                            if (0 == can_open) {//需要拓展slot
                                int sum = helper_->txn_service_->extend_txn_slots(txn->txn_slots, config_->slots);
                                if (0 == sum) {//已到达上限，不能再扩展
                                    return;
                                } else {//persist to redis
                                    helper_->txn_service_->append_txn_slots(r, txn->txn_slots_key, i, txn->txn_slots);
                                }
                                slots_size = static_cast<int>(txn->txn_slots.size());
                                log_trace_g(config_->stgy_id, "need extend slot, new extend slot num={0}, slot size={1}", sum, slots_size);
                            }
                        } while (0 == can_open);
                        if (can_open > 0 && (txn->base_price * sign - open_spread >= price * sign)) {
                            log_trace_g(config_->stgy_id, "vol_uplimit={0}, can_open={1}, base_price={2}, open_spread={3}, price={4}, sign={5}",
                                        vol_uplimit, can_open, txn->base_price, open_spread, price, sign);
                            can_open = std::min(can_open, vol_uplimit);
                            auto orders = order_func(helper_->leg0_->order_book_id, can_open, price);
                            if (!orders.empty()) {

                                auto open_group = std::make_shared<OpenGroup>();
                                open_group->slot_index = i;
                                open_group->group_type = group::GROUP_TYPE_TYPE::NORMAL;
                                helper_->add_open_group(r, txn, open_group, orders[0]);
                                log_trace_g(config_->stgy_id, open_group->to_string());
                                helper_->order_service_->add_order(r, orders[0]);

                                txn->opening_count += can_open;
                                helper_->txn_service_->update_txn(r, *txn);
                                txn->txn_slots[i]->opening = can_open;
                                helper_->txn_service_->set_txn_slot(r, txn->txn_slots_key, i, *txn->txn_slots[i]);
                            }
                        }
                    }
                    break;
                }
                default:
                    ;
            }
        }

        void SingleGridStgy::buy_open(cpp_redis::client &r, std::shared_ptr<Tick> tick) {
//            log_trace_g(config_->stgy_id, "buy_open");
            {
                helper_->long_txn_sm_->guard();
                open(r, tick->a1, tick->a1_v, sign::POS, helper_->long_txn_,
                     [&](const std::string &order_book_id, int amount, double price) {
                         return api_future_->buy_open(order_book_id, amount, price);
                     });
                r.commit();
            }
            {
                helper_->short_txn_sm_->guard();
                lock(r, tick->a1, tick->a1_v, sign::POS, helper_->short_txn_,
                     [&](const std::string &order_book_id, int amount, double price) {
                         return api_future_->buy_open(order_book_id, amount, price);
                     });
                r.commit();
            }
        }
        void SingleGridStgy::sell_open(cpp_redis::client &r, std::shared_ptr<Tick> tick) {
//            log_trace_g(config_->stgy_id, "sell_open");
            {
                helper_->short_txn_sm_->guard();
                open(r, tick->b1, tick->b1_v, sign::NEG, helper_->short_txn_,
                     [&](const std::string &order_book_id, int amount, double price) {
                         return api_future_->sell_open(order_book_id, amount, price);
                     });
                r.commit();
            }
            {
                helper_->long_txn_sm_->guard();
                lock(r, tick->b1, tick->b1_v, sign::NEG, helper_->long_txn_,
                     [&](const std::string &order_book_id, int amount, double price) {
                         return api_future_->sell_open(order_book_id, amount, price);
                     });
                r.commit();
            }
        }

        void SingleGridStgy::close(cpp_redis::client &r, double price, int volume, int sign, TxnPtr txn, order_func_t order_func) {
            switch (txn->status) {
                case txn::STATUS_TYPE::STOP: {
                    break;
                }
                case txn::STATUS_TYPE::ONLY_CLOSE:
//                case txn::STATUS_TYPE::SEMI_LOCK:
                case txn::STATUS_TYPE::RUNNING: {
                    auto vol_uplimit = static_cast<int>(volume * config_->vol_use_rate);
                    if (vol_uplimit <= 0) {
                        return;
                    }
                    if (txn->closing_count > 0) {//有正在平仓的group，直接返回
                        return;
                    }
                    if (txn->txn_costs.size() == 1) {//最后一单不平，但会更新基准价
                        if (txn->base_price * sign < price * sign) {
                            txn->base_price = price;
                            helper_->txn_service_->update_txn_lock(r, *txn);
                        }
                        return;
                    }
                    auto close_group = std::make_shared<CloseGroup>();
                    size_t i = 0, txn_costs_size = txn->txn_costs.size();
                    for (auto &txn_cost : txn->txn_costs) {
                        if (++i >= txn_costs_size) break;//最后一单不平
                        auto &open_group = txn_cost->open_group;
                        if (price * sign - config_->close_spread >= txn_cost->avg_cost * sign) {
                            if (helper_->scan_can_close(r, open_group, close_group, vol_uplimit)) {
                                break;
                            }
                        } else {
                            break;
                        }
                    }
                    if (close_group->unit_quantity > 0) {
                        log_trace_g(config_->stgy_id, close_group->to_string());
                        auto orders = order_func(helper_->leg0_->order_book_id, close_group->unit_quantity, price);
                        if (!orders.empty()) {
                            close_group->avg_cost = price;
                            close_group->group_type = group::GROUP_TYPE_TYPE::NORMAL;
                            helper_->add_close_group(r, txn, close_group, orders);
                            txn->closing_count = close_group->unit_quantity;
                            helper_->txn_service_->update_txn(r, *txn);
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
        void SingleGridStgy::unlock(cpp_redis::client &r, double price, int volume, int , TxnPtr txn, order_func_t order_func) {
            switch (txn->status) {
                case txn::STATUS_TYPE::UNLOCKING: {
                    log_trace_g(config_->stgy_id, "closing_count={0}, lock_count={1}", txn->closing_count, txn->lock_count);
                    auto vol_uplimit = static_cast<int>(volume * config_->vol_use_rate);
                    if (vol_uplimit <= 0) {
                        return;
                    }
                    if (txn->closing_count > 0) {//有正在平仓的group，直接返回
                        return;
                    }
                    if (txn->lock_count <= 0) {
                        return;
                    }
                    auto close_group = std::make_shared<CloseGroup>();
                    for (auto &txn_lock : txn->txn_locks) {
                        auto &open_group = txn_lock->open_group;
                        log_trace_g(config_->stgy_id, open_group->to_string());
                        if (helper_->scan_can_close(r, open_group, close_group, vol_uplimit)) {
                            break;
                        }
                    }
                    log_trace_g(config_->stgy_id, close_group->to_string());
                    if (close_group->unit_quantity > 0) {
                        txn->closing_count = close_group->unit_quantity;
                        helper_->txn_service_->update_txn_lock(r, *txn);
                        auto orders = order_func(helper_->leg0_->order_book_id, close_group->unit_quantity, price);
                        if (!orders.empty()) {
                            close_group->avg_cost = price;
                            close_group->group_type = group::GROUP_TYPE_TYPE::LOCK;
                            helper_->add_close_group(r, txn, close_group, orders);
                        }
                    }
                    break;
                }
                default:
                    ;
            }
        }

        void SingleGridStgy::sell_close(cpp_redis::client &r, std::shared_ptr<Tick> tick) {
//            log_trace_g(config_->stgy_id, "sell_close");
            {
                helper_->long_txn_sm_->guard();
                close(r, tick->b1, tick->b1_v, sign::POS, helper_->long_txn_,
                      [&](const std::string &order_book_id, int amount, double price) {
                          return api_future_->sell_close(order_book_id, amount, price, config_->close_today);
                      });
                r.commit();
            }
            {
                helper_->short_txn_sm_->guard();
                unlock(r, tick->b1, tick->b1_v, sign::POS, helper_->short_txn_,
                       [&](const std::string &order_book_id, int amount, double price) {
                           return api_future_->sell_close(order_book_id, amount, price, config_->close_today);
                       });
                r.commit();
            }
        }
        void SingleGridStgy::buy_close(cpp_redis::client &r, std::shared_ptr<Tick> tick) {
//            log_trace_g(config_->stgy_id, "buy_close");
            {
                helper_->short_txn_sm_->guard();
                close(r, tick->a1, tick->a1_v, sign::NEG, helper_->short_txn_,
                      [&](const std::string &order_book_id, int amount, double price) {
                          return api_future_->buy_close(order_book_id, amount, price, config_->close_today);
                      });
                r.commit();
            }
            {
                helper_->long_txn_sm_->guard();
                unlock(r, tick->a1, tick->a1_v, sign::NEG, helper_->long_txn_,
                       [&](const std::string &order_book_id, int amount, double price) {
                           return api_future_->buy_close(order_book_id, amount, price, config_->close_today);
                       });
                r.commit();
            }
        }

        void SingleGridStgy::handle_tick(StgyContext *, std::shared_ptr<Tick> tick) {
//            log_trace_g(config_->stgy_id, "SingleGridStgy::handle_tick");

            auto r = RedisFactory::get_redis();
//            auto tick1 = Env::instance()->snapshot[helper_->leg0_];

            handle_txn_status(*r, tick, sign::POS, *(helper_->long_txn_));
            handle_txn_status(*r, tick, sign::NEG, *(helper_->short_txn_));

            buy_open(*r, tick);
            sell_close(*r, tick);
            sell_open(*r, tick);
            buy_close(*r, tick);


            log_trace_g(config_->stgy_id, "tick={0}", tick->to_string());
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

        void SingleGridStgy::handle_txn_status(cpp_redis::client &r, std::shared_ptr<Tick> tick, int sign, Txn &txn) {
            switch (txn.status) {
                case txn::STATUS_TYPE::LOCK: {
/*                    log_info_g(config_->stgy_id, "sign={0}, lock_price={1}, open_spread={2}, last={3}, old_ma20_ols={4}",
                               sign, txn.lock_price, config_->open_spread, tick->last, txn.old_ma20_ols);*/
                    if ((txn.lock_price * sign + config_->open_spread < tick->last * sign)
                        && txn.old_ma20_ols * sign > 0) {
                        txn.status = txn::STATUS_TYPE::UNLOCKING;
                        txn.threshold_price = 0;
                        helper_->txn_service_->update_txn_lock(r, txn);
                        log_info_g(config_->stgy_id,
                                   "update_signal, last price regress, set semi_lock -> unlocking, direct={0}, threshold_price={1}, last={2}",
                                   txn::s_direct_type.enum2literal(txn.direct), txn.threshold_price, tick->last);
                    }
                    break;
                }
                default:
                    ;
            }
        }

        void SingleGridStgy::before_trading(StgyContext *) {
            log_trace_g(config_->stgy_id, "SingleGridStgy::before_trading");
        }

        void SingleGridStgy::after_trading(StgyContext *) {
            log_trace_g(config_->stgy_id, "SingleGridStgy::after_trading");
        }

        void SingleGridStgy::post_after_trading(StgyContext *) {
            static bool flag = false;
            if (!flag) {
                log_info_g(config_->stgy_id, "SingleGridStgy::post_after_trading");
                flag = true;
            }
        }

    } /* stgy  */
} /* iqt */ 
