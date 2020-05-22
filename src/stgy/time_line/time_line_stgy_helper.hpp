#ifndef _IQT_STGY_TIME_LINE_TIME_LINE_STGY_HELPER_HPP_
#define _IQT_STGY_TIME_LINE_TIME_LINE_STGY_HELPER_HPP_

#include <string>
#include <vector>
#include <list>


#include "trade/abs_broker.h"

#include "service/order_service.hpp"
#include "service/group_service.hpp"
#include "service/txn_service.hpp"
#include "service/rpt_service.hpp"
#include "service/stgy_service.hpp"

#include "model/instrument.h"

#include "base/lock/spin_mutex.hpp"
#include "const.hpp"

namespace iqt {
    using namespace model;
    using namespace service;

    namespace stgy {
        struct TimeLineConfig {
            std::string stgy_type;
            std::string stgy_id;
            std::string version;
            std::string log_level = "info";
            std::vector<std::string> universe;
            double starting_cash;
            double odds;
            double spread;
            int slot;
            bool close_today;
            double vol_use_rate = 1;
        };

        class TimeLineStgy;

        class TimeLineStgyHelper {
            friend class TimeLineStgy;
        public:
            explicit TimeLineStgyHelper(const TimeLineConfig *conf) :
                    config_(conf), stgy_(new Stgy()), rpt_(new Rpt()),
                    stgy_service_(new StgyService()), rpt_service_(new RptService()),
                    txn_service_(new TxnService(conf->stgy_id)), group_service_(new GroupService(conf->stgy_id, LEG_NUM)),
                    order_service_(new OrderService()),
                    long_txn_sm_(new base::lock::SpinMutex()), short_txn_sm_(new base::lock::SpinMutex()),
                    leg0_(get_leg(0)) {}

            ~TimeLineStgyHelper() {
                delete stgy_;
                delete rpt_;
                delete stgy_service_;
                delete rpt_service_;
                delete txn_service_;
                delete group_service_;
                delete order_service_;
                delete long_txn_sm_;
                delete short_txn_sm_;
            }
            InstrumentPtr get_leg(size_t index) {
//                PPK_ASSERT_ERROR(universe.size() > index, "size=%d, index=%d", (int)universe.size(), (int)index);
                assert(config_->universe.size() > index);
                assert(Env::instance()->instruments->count(config_->universe[index]));
                return Env::instance()->instruments->at(config_->universe[index]);
            }

            const std::string &get_rpt_key() const {
                return stgy_->rpt_key;
            }


            void load_stgy(cpp_redis::client &r) {
                stgy_->stgy_type = config_->stgy_type;
                stgy_->stgy_id = config_->stgy_id;
                stgy_->version = config_->version;
                stgy_->account_id = Env::instance()->broker->get_account_id();

                static bool stgy_flag = false;
                if (!stgy_flag) {
                    stgy_service_->clear_handling_stgy(r, *stgy_);
                    stgy_flag = true;
                }
                stgy_service_->load_stgy(r, *stgy_);
                log_info_g(config_->stgy_id, "load_stgy successful");
            }

            void load_rpt(cpp_redis::client &r) {
                rpt_->rpt_key = stgy_->rpt_key;
                rpt_->stgy_id = stgy_->stgy_id;
                rpt_service_->load_rpt(r, *rpt_);
                log_info_g(config_->stgy_id, "load_rpt successful");
            }

            ///////////////txn start///////////////////////////////////


            void load_long_txn(cpp_redis::client &r) {
                rpt_->long_txn_key = rpt_service_->get_handling_txn_key(r, rpt_->rpt_key, rpt::LONG_TXN_KEY);
                long_txn_ = std::make_shared<Txn>(rpt_->long_txn_key, stgy_->stgy_id, txn::DIRECT_TYPE::LONG);
                txn_service_->load_txn(r, *long_txn_);
                if (txn::STATUS_TYPE::NEW == long_txn_->status) {
                    long_txn_->status = txn::STATUS_TYPE::RUNNING;
                }
                assert(long_txn_->closing_count == 0);
                assert(long_txn_->opening_count == 0);
                std::vector<int> slots{config_->slot};

                long_txn_->txn_slots = txn_service_->get_txn_slots(r, long_txn_->txn_slots_key, slots);
                long_txn_->txn_costs = txn_service_->get_long_txn_costs(r, long_txn_->txn_costs_key);//加载开仓groups
//                txn_service_->check_handling_group(r, long_txn_->txn_key);
                log_info_g(config_->stgy_id, "visit long_txn_costs_======================================");
                for (auto &item : long_txn_->txn_costs) {
                    group_service_->get_open_group(r, item->open_group);
                    assert(!item->open_group->closing_unit_quantity);
//                    PPK_ASSERT_ERROR(!item->open_group->closing_unit_quantity, "open_group的正在平仓数不为0, group_key=%s", item->open_group->group_key.c_str());
                    log_info_g(config_->stgy_id, item->open_group->to_string());
                }
                long_txn_->txn_locks = txn_service_->get_long_txn_costs(r, long_txn_->txn_locks_key);//加载锁仓groups
                log_info_g(config_->stgy_id, "visit long_txn_locks_======================================");
                for (auto &item : long_txn_->txn_locks) {
                    group_service_->get_open_group(r, item->open_group);
                    assert(!item->open_group->closing_unit_quantity);
                    item->open_group->txn = long_txn_;
//                    PPK_ASSERT_ERROR(!item->open_group->closing_unit_quantity, "open_group的正在平仓数不为0, group_key=%s", item->open_group->group_key.c_str());
                    log_info_g(config_->stgy_id, item->open_group->to_string());
                }
                log_info_g(config_->stgy_id, "load_long_txn successful");
            }

            void load_short_txn(cpp_redis::client &r) {
                rpt_->short_txn_key = rpt_service_->get_handling_txn_key(r, rpt_->rpt_key, rpt::SHORT_TXN_KEY);
                short_txn_ = std::make_shared<Txn>(rpt_->short_txn_key, stgy_->stgy_id, txn::DIRECT_TYPE::SHORT);
                txn_service_->load_txn(r, *short_txn_);
                if (txn::STATUS_TYPE::NEW == short_txn_->status) {
                    short_txn_->status = txn::STATUS_TYPE::RUNNING;
                }
                assert(long_txn_->closing_count == 0);
                assert(long_txn_->opening_count == 0);
                std::vector<int> slots{config_->slot};
                short_txn_->txn_slots = txn_service_->get_txn_slots(r, short_txn_->txn_slots_key, slots);
                short_txn_->txn_costs = txn_service_->get_short_txn_costs(r, short_txn_->txn_costs_key);//加载开仓groups
//                txn_service_->check_handling_group(r, short_txn_->txn_key);
                log_info_g(config_->stgy_id, "visit short_txn_costs_======================================");
                for (auto &item : short_txn_->txn_costs) {
                    group_service_->get_open_group(r, item->open_group);
                    assert(!item->open_group->closing_unit_quantity);
                    item->open_group->txn = short_txn_;
//                    PPK_ASSERT_ERROR(!item->open_group->closing_unit_quantity, "open_group的正在平仓数不为0, group_key=%s", item->open_group->group_key.c_str());
                    log_info_g(config_->stgy_id, item->open_group->to_string());
                }
                short_txn_->txn_locks = txn_service_->get_short_txn_costs(r, short_txn_->txn_locks_key);//加载锁仓groups
                log_info_g(config_->stgy_id, "visit short_txn_locks_======================================");
                for (auto &item : short_txn_->txn_locks) {
                    group_service_->get_open_group(r, item->open_group);
                    assert(!item->open_group->closing_unit_quantity);
//                    PPK_ASSERT_ERROR(!item->open_group->closing_unit_quantity, "open_group的正在平仓数不为0, group_key=%s", item->open_group->group_key.c_str());
                    log_info_g(config_->stgy_id, item->open_group->to_string());
                }
                log_info_g(config_->stgy_id, "load_short_txn successful");
            }

            void reset_txn_slots(cpp_redis::client &r, const std::string &txn_slots_key, std::vector<SlotPtr> &txn_slots, int closed_num) {
                for (auto i = txn_slots.size(); i-- > 0; ) {
                    auto &slot = *(txn_slots[i]);
                    if (slot.open > 0) {
                        if (closed_num <= 0) break;
                        if (closed_num >= slot.open) {
                            closed_num -= slot.open;
                            slot.open = 0;
                        } else {
                            slot.open -= closed_num;
                            closed_num = 0;
                        }
                        txn_service_->set_txn_slot(r, txn_slots_key, i, slot);
                    }
                }
            }

            void remove_txn_cost(cpp_redis::client &r, const std::string &txn_costs_key, std::list<TxnCostPtr> &txn_costs, OpenGroupPtr open_group) {
                txn_costs.remove_if([&](const TxnCostPtr & elem) {
                    return elem->open_group->group_key == open_group->group_key;
                });
                txn_service_->remove_txn_cost(r, txn_costs_key, open_group->group_key);
            }
            void add_txn_cost(cpp_redis::client &r, int sign, const std::string &txn_costs_key, std::list<TxnCostPtr> &txn_costs, OpenGroupPtr open_group) {
                auto txn_cost = std::make_shared<TxnCost>(open_group, open_group->avg_cost);
                auto iter = std::find_if(txn_costs.begin(), txn_costs.end(), [&](const TxnCostPtr &elem) {
                    return txn_cost->avg_cost * sign <= elem->avg_cost * sign;
                });
                txn_service_->add_txn_cost(r, txn_costs_key, txn_cost);
                txn_costs.insert(iter, std::move(txn_cost));
            }
            ///////////////txn end///////////////////////////////////

            ///////////////group start///////////////////////////////////
            void add_open_group(cpp_redis::client &r, TxnPtr txn, OpenGroupPtr open_group, OrderPtr ord) {
//                auto open_group = std::make_shared<OpenGroup>();
//                open_group->group_type = group_type;
                open_group->txn = txn;
                open_group->unit_quantity = ord->quantity;
                open_group->avg_cost = ord->price;
//                open_group->slot_index = slot_index;
                auto leg = std::make_shared<Leg>();
                leg->filled_quantity = 0;
                leg->avg_price = ord->price;
                leg->frag_status = leg::FRAG_STATUS_TYPE::NO;
                leg->orders.push_back(ord);
                open_group->leg_items.push_back(std::move(leg));
                group_service_->add_open_group(r, open_group);
                ord->group_key = open_group->group_key;
                txn_service_->set_handle_group_key(r, txn->txn_key, txn::OPEN_GROUP_KEY, open_group->group_key, txn->txn_key + suffix::OPEN_GROUP);
            }
            void add_close_group(cpp_redis::client &r, TxnPtr txn, CloseGroupPtr close_group, const std::vector<OrderPtr> &orders) {
                close_group->txn = txn;
                auto leg = std::make_shared<Leg>();
                int quan = 0;
                for (auto &ord : orders) {
                    quan += ord->quantity;
                    leg->orders.push_back(ord);
                }
//                PPK_ASSERT(quan == close_group->unit_quantity, "实际平仓手数[%d]小于预期平仓手数[%d]", quan, close_group->unit_quantity);
                assert(quan == close_group->unit_quantity);
                if (orders.size() >= 2) {
                    leg->frag_status = leg::FRAG_STATUS_TYPE::NEED;
                } else {
                    leg->frag_status = leg::FRAG_STATUS_TYPE::NO;
                }
                leg->filled_quantity = 0;
                leg->avg_price = close_group->avg_cost;
                close_group->leg_items.push_back(std::move(leg));
                group_service_->add_close_group(r, close_group);
                for (auto &ord : orders) {
                    ord->group_key = close_group->group_key;
                    order_service_->add_order(r, ord);
                }
                txn_service_->set_handle_group_key(r, txn->txn_key, txn::CLOSE_GROUP_KEY, close_group->group_key, txn->txn_key + suffix::CLOSE_GROUP);
            }
            OpenGroupPtr update_open_group(cpp_redis::client &r, Order &ord) {
                auto open_group = group_service_->get_open_group(r, ord.group_key);
                open_group->filled_unit_quantity = ord.filled_quantity;
                auto &leg = open_group->leg_items[0];
                leg->filled_quantity = ord.filled_quantity;
                group_service_->update_open_group(r, *open_group);
                return open_group;
            }
            CloseGroupPtr update_close_group(cpp_redis::client &r, Order &ord) {
                auto close_group = group_service_->get_close_group(r, ord.group_key);
                close_group->filled_unit_quantity += ord.filled_quantity;
                auto &leg = close_group->leg_items[0];
                leg->filled_quantity += ord.filled_quantity;
                switch (leg->frag_status) {
                    case leg::FRAG_STATUS_TYPE::NEED: {
                        leg->frag_status = leg::FRAG_STATUS_TYPE::RCV_ONE;
                        break;
                    }
                    case leg::FRAG_STATUS_TYPE::RCV_ONE: {
                        leg->frag_status = leg::FRAG_STATUS_TYPE::RCV_TWO;
                        break;
                    }
                    default:
                        ;
                }
                group_service_->update_close_group(r, *close_group);
                return close_group;
            }


            bool scan_can_close(cpp_redis::client &r, OpenGroupPtr open_group, CloseGroupPtr close_group, int &vol_uplimit) {//vol_uplimit引用符号不能去掉
                int remain = open_group->filled_unit_quantity - open_group->close_unit_quantity;
                bool flag = false;
                if (remain >= vol_uplimit) {
                    close_group->unit_quantity += vol_uplimit;
                    close_group->open_groups.push_back(open_group);
                    open_group->closing_unit_quantity += vol_uplimit;
                    vol_uplimit = 0;
                    flag = true;
                } else {
                    close_group->unit_quantity += remain;
                    close_group->open_groups.push_back(open_group);
                    open_group->closing_unit_quantity += remain;
                    vol_uplimit -= remain;
                }
                group_service_->update_open_group_quantity(r, *open_group);
                return flag;
            }

            OpenGroupPtr update_open_group_after_trade(cpp_redis::client &r, TradeTuple &trade) {
                auto &ord = std::get<1>(trade);
                auto open_group = group_service_->get_open_group(r, ord->group_key);
                open_group->avg_cost = ord->avg_price;
                open_group->transaction_cost = ord->transaction_cost;
                auto &leg = open_group->leg_items[0];
                leg->avg_price = ord->avg_price;
                leg->transaction_cost = ord->transaction_cost;
                group_service_->update_open_group_after_trade(r, *open_group);

//                auto &txn = open_group->txn;
                return open_group;
            }
            CloseGroupPtr update_close_group_after_trade(cpp_redis::client &r, TradeTuple &trade) {
                auto &ord = std::get<1>(trade);
                auto close_group = group_service_->get_close_group(r, ord->group_key);
                close_group->avg_cost = ord->avg_price;
                close_group->transaction_cost = ord->transaction_cost;
                auto &leg = close_group->leg_items[0];
                leg->avg_price = ord->avg_price;
                leg->transaction_cost = ord->transaction_cost;
                group_service_->update_close_group_after_trade(r, *close_group);
                return close_group;
            }

            double calc_holding_pnl(int sign, Txn &txn) {
                double pnl = 0;
                auto tick = Env::instance()->snapshot[leg0_->order_book_id];
                if (!tick) return pnl;
                auto last = tick->last;
                for (auto &cost : txn.txn_costs) {
                    auto remain = cost->open_group->filled_unit_quantity - cost->open_group->close_unit_quantity;
                    pnl += sign * (last - cost->open_group->avg_cost) * remain * leg0_->contract_multiplier;
                }
                for (auto &lock : txn.txn_locks) {
                    auto remain = lock->open_group->filled_unit_quantity - lock->open_group->close_unit_quantity;
                    pnl += sign * (lock->open_group->avg_cost - last) * remain * leg0_->contract_multiplier;
                }
                return pnl;
            }

            void update_open_pnl_cost(cpp_redis::client &r, int sign, OpenGroup &open_group) {

                auto &txn = open_group.txn;
                txn->holding_pnl = calc_holding_pnl(sign, *txn);
                txn->transaction_cost += open_group.transaction_cost;
                txn_service_->update_txn(r, *txn);

                rpt_->holding_pnl = calc_holding_pnl(sign::POS, *long_txn_) + calc_holding_pnl(sign::NEG, *short_txn_);
                rpt_->transaction_cost += open_group.transaction_cost;
                rpt_service_->update_rpt(r, *rpt_);
            }

            void update_close_pnl_cost(cpp_redis::client &r, int sign, CloseGroup &close_group) {
                group_service_->update_close_group_pnl(r, close_group);

                auto &txn = close_group.txn;
                if (close_group.realized_pnl - close_group.transaction_cost < 0) {
                    txn->loss_count += 1;
                    rpt_->loss_count += 1;
                }
                txn->realized_pnl += close_group.realized_pnl;
                txn->holding_pnl = calc_holding_pnl(sign, *txn);
                txn->transaction_cost += close_group.transaction_cost;
                txn_service_->update_txn(r, *txn);

                rpt_->realized_pnl += close_group.realized_pnl;
                rpt_->holding_pnl = calc_holding_pnl(sign::POS, *long_txn_) + calc_holding_pnl(sign::NEG, *short_txn_);
                rpt_->transaction_cost += close_group.transaction_cost;
                rpt_service_->update_rpt(r, *rpt_);
            }
            ///////////////group end///////////////////////////////////

            ///////////////order start///////////////////////////////////
            ///////////////order end///////////////////////////////////

            ///////////////signal start///////////////////////////////////

            void update_signal(std::shared_ptr<model::Tick> tick) {
                auto r = RedisFactory::get_redis();
                {
                    long_txn_sm_->guard();
                    update_signal(*r, *long_txn_, sign::POS, tick);
                    r->commit();
                }
                {
                    short_txn_sm_->guard();
                    update_signal(*r, *short_txn_, sign::NEG, tick);
                    r->commit();
                }
            }

            void update_signal(cpp_redis::client &r, Txn &txn, int sign, std::shared_ptr<model::Tick> tick) {
                tick->fix_settlement();
                auto price = sign::POS == sign ? tick->a1 : tick->b1;
                switch (txn.status) {
                    case txn::STATUS_TYPE::NEW:
                    case txn::STATUS_TYPE::LOCK: {

                        if (sign * tick->settlement + config_->spread <= sign * price) {
                            txn.status = txn::STATUS_TYPE::UNLOCKING;
                            txn_service_->update_txn(r, txn);
                        }
                        break;
                    }
                    default:;
                }
            }

            ///////////////signal end///////////////////////////////////
        private:
            const TimeLineConfig *config_;
            Stgy *stgy_;
            Rpt *rpt_;
            service::StgyService *stgy_service_;
            service::RptService *rpt_service_;
            service::TxnService *txn_service_;
            service::GroupService *group_service_;
            service::OrderService *order_service_;

            base::lock::SpinMutex *long_txn_sm_;
            base::lock::SpinMutex *short_txn_sm_;
        private:
            TxnPtr long_txn_;
//            std::vector<SlotPtr> long_txn_slots_;
//            std::list<TxnCostPtr> long_txn_costs_;
//            std::list<TxnCostPtr> long_txn_locks_;

            TxnPtr short_txn_;
//            std::vector<SlotPtr> short_txn_slots_;
//            std::list<TxnCostPtr> short_txn_costs_;
//            std::list<TxnCostPtr> short_txn_locks_;

            InstrumentPtr leg0_;
            static const int LEG_NUM = 1;//该策略用到的对冲品种数量
        };

    } /* stgy  */
} /* iqt */
#endif /* end of include guard: _IQT_STGY_TIME_LINE_TIME_LINE_STGY_HELPER_HPP_ */
