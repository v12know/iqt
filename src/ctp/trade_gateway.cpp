#include "trade_gateway.h"

//#include <chrono>
#include <algorithm>
#include <regex>

#include "global.h"
#include "event/event.h"
#include "base/redis.h"
#include "base/lock/spin_mutex.hpp"
#include "base/log.h"
#include "base/exception.h"
#include "base/util.hpp"
#include "ctp/trade_api.h"
#include "env.h"
//#include "md_gateway.h"

#include "model/tick.h"
#include "model/trade.h"
#include "model/instrument.h"
#include "model/account.h"
#include "model/position.h"
#include "model/order.h"
#include "model/future_info.h"
#include "trade/future_account.h"
#include "trade/future_position.h"
#include "trade/portfolio.h"
#include "ctp/ctp_broker.h"
#include "service/future_info_service.hpp"



namespace iqt {
    namespace trade {
        using std::chrono::system_clock;

        DataCache::DataCache() : trades_(std::make_shared<model::TradeListMap>()),
                                 orders_(std::make_shared<model::OrderMap>()),
                                 sys_orders_(std::make_shared<model::OrderMap>()),
                                 future_info_(std::make_shared<model::FutureInfoMap>()) {
//            Env::instance()->future_info = future_info_;
        }

        void DataCache::cache_ins(model::InstrumentMapPtr ins_cache) {
            ins_ = std::move(ins_cache);
            for (auto &item : *ins_) {
                auto p_future_info = std::make_shared<model::FutureInfo>();
                p_future_info->speculation.long_margin_ratio = item.second->long_margin_ratio;
                p_future_info->speculation.short_margin_ratio = item.second->short_margin_ratio;
                p_future_info->speculation.margin_type = item.second->margin_type;
                (*future_info_)[item.first] = p_future_info;
            }
        }

        void
        DataCache::cache_commission(const std::string &order_book_id, std::shared_ptr<model::Commission> commission) {
            auto &p_future_info = (*future_info_)[order_book_id];
            p_future_info->speculation.open_commission_ratio = commission->open_commission_ratio;
            p_future_info->speculation.close_commission_ratio = commission->close_commission_ratio;
            p_future_info->speculation.close_commission_today_ratio = commission->close_commission_today_ratio;
            p_future_info->speculation.commission_type = commission->commission_type;
        }

        void DataCache::cache_position(model::PositionMapPtr position_cache) {
            pos_ = std::move(position_cache);
            std::vector<std::string> tmp_vec;
            for (auto &item : *pos_) {
                if (Env::instance()->snapshot.count(item.first) == 0) {
                    auto tick = std::make_shared<model::Tick>(item.second->pre_settlement_price);
                    Env::instance()->snapshot[item.first] = tick;
//                    Env::instance()->snapshot.emplace(item.first, tick);
                }
                tmp_vec.push_back(item.first);
            }
/*            auto event = std::make_shared<event::Event<std::vector<std::string> &>>(event::POST_UNIVERSE_CHANGED, tmp_vec);
            event::EventBus::publish_event(event);*/
        }

        void DataCache::cache_account(std::shared_ptr<model::Account> account) {
            account_ = std::move(account);
        }

        void DataCache::cache_trade(std::shared_ptr<model::Trade> trade) {
//            trades_->emplace(trade->order_book_id, trade);
            (*trades_)[trade->order_book_id].push_back(trade);
        }

        void DataCache::cache_open_order(std::shared_ptr<model::Order> order) {
            auto iter = std::find_if(open_orders_.cbegin(), open_orders_.cend(),
                                     [&order](const std::shared_ptr<model::Order> o) {
                                         return order->order_key == o->order_key;
                                     });
            if (iter == open_orders_.cend()) {
                open_orders_.push_back(order);
            }
        }

        void DataCache::remove_open_order(std::shared_ptr<model::Order> order) {
            open_orders_.remove_if(
                    [&order](const std::shared_ptr<model::Order> o) {
                        return order->order_key == o->order_key;
                    });
        }

        std::shared_ptr<model::Order> DataCache::get_cached_order(std::shared_ptr<model::Order> order) {
            std::shared_ptr<model::Order> cached_order = nullptr;
            auto iter = orders_->find(order->order_key);
            if (iter != orders_->end()) {
                cached_order = iter->second;

                if (sys_orders_->count(order->order_sys_key) == 0) {
                    cached_order->set_order_sys_key(order->exchange_id, order->order_sys_id);
                    cache_sys_order(cached_order);
                }
            } else {
                cached_order = std::make_shared<model::Order>(order->order_book_id, order->quantity, order->side,
                                                            ORDER_TYPE::LIMIT, order->position_effect, order->price);
                cached_order->set_order_sys_key(order->exchange_id, order->order_sys_id);
                cached_order->set_order_key(order->front_id, order->session_id, order->order_id);
                cache_sys_order(cached_order);
                cache_order(cached_order);
            }
            return cached_order;
        }

        std::shared_ptr<model::Order> DataCache::get_cached_order(std::shared_ptr<model::Trade> trade) {
            std::shared_ptr<model::Order> cached_order = nullptr;
            auto iter = sys_orders_->find(trade->order_sys_key);
            if (iter != sys_orders_->end()) {
                cached_order = iter->second;
            } else {
                cached_order = std::make_shared<model::Order>(trade->order_book_id, trade->last_quantity, trade->side,
                                                            ORDER_TYPE::LIMIT, trade->position_effect, trade->last_price);
                cached_order->order_sys_id = trade->order_sys_id;
                cached_order->exchange_id = trade->exchange_id;
                cached_order->order_sys_key = trade->order_sys_key;
                cache_sys_order(cached_order);
                cache_order(cached_order);
                cached_order = cached_order;
            }
            return cached_order;
        }

        void DataCache::cache_order(std::shared_ptr<model::Order> order) {
//            orders_->emplace(order->order_key, order);
            (*orders_)[order->order_key] = order;
        }

        void DataCache::cache_sys_order(std::shared_ptr<model::Order> order) {
//            sys_orders_->emplace(order->order_sys_key, order);
            (*sys_orders_)[order->order_sys_key] = order;
        }

        static void process_today_holding_list(int today_quantity, std::list<std::pair<double, int>> &holding_list) {
            if (holding_list.empty()) {
                return;
            }
            int cum_quantity = 0;
            for (auto &pair : holding_list) {
                cum_quantity += pair.second;
            }
            int left_quantity = cum_quantity - today_quantity;
            int consumed_quantity = 0;
            while (left_quantity > 0) {
                auto &pair = holding_list.back();
                double oldest_price = pair.first;
                int oldest_quantity = pair.second;
                holding_list.pop_back();
                if (oldest_quantity > left_quantity) {
                    consumed_quantity = left_quantity;
                    holding_list.emplace_back(oldest_price, oldest_quantity - left_quantity);
                } else {
                    consumed_quantity = oldest_quantity;
                }
                left_quantity -= consumed_quantity;
            }
        }

        std::shared_ptr<trade::Positions> DataCache::positions() {
            auto ps = std::make_shared<trade::Positions>();
            for (auto &item : *pos_) {
                auto &order_book_id = item.first;
                auto &position = ps->get_or_create(order_book_id);
                auto &pos_dict = item.second;
                log_trace("pos_dict: {0}", pos_dict->to_string());
                position->set_buy_old_holding_list({{pos_dict->pre_settlement_price, pos_dict->buy_old_quantity}});
                position->set_sell_old_holding_list({{pos_dict->pre_settlement_price, pos_dict->sell_old_quantity}});

                position->set_buy_transaction_cost(pos_dict->buy_transaction_cost);
                position->set_sell_transaction_cost(pos_dict->sell_transaction_cost);
                position->set_buy_realized_pnl(pos_dict->buy_realized_pnl);
                position->set_sell_realized_pnl(pos_dict->sell_realized_pnl);

                position->set_buy_avg_open_price(pos_dict->buy_avg_open_price);
                position->set_sell_avg_open_price(pos_dict->sell_avg_open_price);
                assert(trades_);
                auto iter = trades_->find(order_book_id);
                if (iter != trades_->end()) {
                    iter->second.sort([](const std::shared_ptr<model::Trade> lhs, const std::shared_ptr<model::Trade> rhs) {
                        return lhs->trade_id > rhs->trade_id;
                    });
                    std::list<std::pair<double, int>> buy_today_holding_list;
                    std::list<std::pair<double, int>> sell_today_holding_list;
                    for (auto &trade_dict : iter->second) {
                        if (trade_dict->position_effect == trade::POSITION_EFFECT_TYPE::OPEN) {
                            if (trade_dict->side == trade::SIDE_TYPE::BUY) {
                                buy_today_holding_list.emplace_back(trade_dict->last_price, trade_dict->last_quantity);
                            } else {
                                sell_today_holding_list.emplace_back(trade_dict->last_price, trade_dict->last_quantity);
                            }
                        }
                    }

                    process_today_holding_list(pos_dict->buy_today_quantity, buy_today_holding_list);
                    process_today_holding_list(pos_dict->sell_today_quantity, sell_today_holding_list);
                    position->set_buy_today_holding_list(std::move(buy_today_holding_list));
                    position->set_sell_today_holding_list(std::move(sell_today_holding_list));
                }
                log_trace("sell_quantity={0}, buy_quantity={1}", position->sell_quantity(), position->buy_quantity());
            }

            return ps;
        }

        std::pair<std::shared_ptr<trade::FutureAccount>, double> DataCache::account() {
            double static_value = account_->pre_balance ? account_->pre_balance : account_->balance;
            auto ps = positions();
            double realized_pnl = 0, cost = 0, margin = 0;
            for (auto &item : *ps) {
                realized_pnl += item.second->realized_pnl();
                cost += item.second->transaction_cost();
                margin += item.second->margin();
            }
            double total_cash = static_value + realized_pnl - cost - margin;
            auto future_account = std::make_shared<trade::FutureAccount>(total_cash, ps);
            double frozen_cash = 0;
            for (auto &item : *orders_) {
                if (item.second->status == trade::ORDER_STATUS_TYPE::ACTIVE) {
                    frozen_cash += global::margin_of(item.second->order_book_id, item.second->quantity - item.second->filled_quantity, item.second->price);
                }
            }
            future_account->set_frozen_cash(frozen_cash);
            return {future_account, static_value};
        }
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////
        CtpTradeGateway::CtpTradeGateway(int retry_times, int retry_interval) :
                cache_(new DataCache()), retry_times_(retry_times), retry_interval_(retry_interval),
                sm_(new base::lock::SpinMutex()) {}

        CtpTradeGateway::~CtpTradeGateway() {
            delete cache_;
            delete td_api_;
            delete sm_;
        }

        void CtpTradeGateway::init_env() {
            Env::instance()->snapshot.reserve(cache_->ins_->size());
            Env::instance()->instruments = cache_->ins_;
            Env::instance()->trading_date = get_trading_date();
//            log_warn("!!!!!!!!!!!!!!!!trading_date={0}!!!!!!!!!!!!!!!!!!!!!!!!!", Env::instance()->trading_date);
//            exit(0);
            if (Env::instance()->config.run_type != RUN_TYPE::COLLECT) {
                Env::instance()->broker = std::make_shared<CtpBroker>(this);
                Env::instance()->portfolio = Env::instance()->broker->get_portfolio();
            }
        }

        void
        CtpTradeGateway::connect(const std::string &broker_id, const std::string &user_id, const std::string &password,
                              const std::string &address, const std::string &pub_resume_type,
                              const std::string &priv_resume_type) {
            delete td_api_;
            td_api_ = new trade::CtpTradeApi(this, broker_id, user_id, password, address, pub_resume_type,
                                           priv_resume_type);
            account_id_ = broker_id + ":" + user_id;
            bool success = false;
            for (auto i = 0; i < retry_times_; ++i) {
                td_api_->connect();
//                sleep(retry_interval_ * (i + 1));
                wait_for();
                if (td_api_->logged_in_) {
                    log_warn("CTP 交易服务器登录成功");
                    success = true;
                    break;
                }
            }
            log_trace("after connect");
            if (!success) {
                MY_THROW(base::BaseException, "CTP 交易服务器连接或登录超时");
            }
//            sleep(1);
            log_info("同步数据中。");
            if (data_update_date_ != base::strftimep(system_clock::now(), "%Y%m%d")) {
                confirm_settlement_info();
                qry_instrument();
                if (Env::instance()->config.run_type != RUN_TYPE::COLLECT) {
                    qry_account();
                    qry_position();
                    qry_order();
                    qry_commission();
//                    exit(1);
                }
                data_update_date_ = base::strftimep(system_clock::now(), "%Y%m%d");
                init_env();
            }
            log_info("数据同步完成。");
        }

        void CtpTradeGateway::join() {
            td_api_->join();
        }

        void CtpTradeGateway::close() {
            if (td_api_) {
                td_api_->close();
                delete td_api_;
                td_api_ = nullptr;
            }
        }

        std::shared_ptr<trade::Portfolio> CtpTradeGateway::get_portfolio() {
            auto pair = cache_->account();
            double static_value = pair.second;
            auto future_account = pair.first;

            std::string start_date = Env::instance()->config.start_date;
            double future_starting_cash = Env::instance()->config.future_starting_cash;
            auto portfolio = std::make_shared<trade::Portfolio>(start_date, static_value / future_starting_cash, future_starting_cash, future_account);
            return portfolio;
        }

        void CtpTradeGateway::submit_order(std::shared_ptr<model::Order> order) {
            sm_->guard();
            event::EventBus::publish_event(
                    std::make_shared<event::Event<model::OrderPtr>>(event::ORDER_PENDING_NEW, order));
            td_api_->send_order(order);
            cache_->cache_order(order);
        }

        void CtpTradeGateway::cancel_order(std::shared_ptr<model::Order> order) {
            sm_->guard();
            cache_->get_cached_order(order)->set_pending_cancel();
            event::EventBus::publish_event(
                    std::make_shared<event::Event<model::OrderPtr>>(event::ORDER_PENDING_CANCEL, order));
            td_api_->cancel_order(order);
        }

        void CtpTradeGateway::on_order(std::shared_ptr<model::Order> ord) {
            sm_->guard();
            log_debug("报单回报: {0}", ord->to_string());
            auto cached_order = cache_->get_cached_order(ord);
//            log_trace("on_order---cached_order: {0}", cached_order->to_string());
            cached_order->filled_quantity = ord->filled_quantity;
            switch (cached_order->status) {
                case ORDER_STATUS_TYPE::PENDING_NEW: {
                    cached_order->order_sys_key = ord->order_sys_key;
                    cached_order->active();
                    event::EventBus::publish_event(
                            std::make_shared<event::Event<model::OrderPtr>>(event::ORDER_CREATION_PASS, cached_order));
                    switch (ord->status) {
                        case ORDER_STATUS_TYPE::ACTIVE: {
                            cache_->cache_open_order(cached_order);
                            break;
                        }
                        case ORDER_STATUS_TYPE::CANCELLED: {
                            cached_order->mark_cancelled("order cancelled");
                            event::EventBus::publish_event(
                                    std::make_shared<event::Event<model::OrderPtr>>(event::ORDER_UNSOLICITED_UPDATE,
                                                                                 cached_order));
                            cache_->remove_open_order(cached_order);
                            break;
                        }
                        case ORDER_STATUS_TYPE::REJECTED: {
                            cached_order->mark_rejected("order rejected");
                            event::EventBus::publish_event(
                                    std::make_shared<event::Event<model::OrderPtr>>(event::ORDER_UNSOLICITED_UPDATE,
                                                                                 cached_order));
                            cache_->remove_open_order(cached_order);
                            break;
                        }
                        case ORDER_STATUS_TYPE::FILLED: {
                            cached_order->status = ord->status;
                            event::EventBus::publish_event(
                                    std::make_shared<event::Event<model::OrderPtr>>(event::ORDER_UNSOLICITED_UPDATE,
                                                                                 cached_order));
                            cache_->remove_open_order(cached_order);
                            break;
                        }
                        default:
                            log_critical("unknown case on cached_order->status=ORDER_STATUS_TYPE::PENDING_NEW, ord->status={0}", base::enum2index(ord->status));
                    }
                    break;
                }
                case ORDER_STATUS_TYPE::ACTIVE: {
                    switch (ord->status) {
                        case ORDER_STATUS_TYPE::FILLED: {
                            cached_order->status = ord->status;
                            event::EventBus::publish_event(
                                    std::make_shared<event::Event<model::OrderPtr>>(event::ORDER_UNSOLICITED_UPDATE,
                                                                                 cached_order));
                            cache_->remove_open_order(cached_order);
                            break;
                        }
                        case ORDER_STATUS_TYPE::CANCELLED: {
                            cached_order->mark_cancelled("order cancelled");
                            event::EventBus::publish_event(
                                    std::make_shared<event::Event<model::OrderPtr>>(event::ORDER_CANCELLATION_PASS,
                                                                                 cached_order));
                            cache_->remove_open_order(cached_order);
                            break;
                        }
                        default:
                            log_critical("unknown case on cached_order->status=ORDER_STATUS_TYPE::ACTIVE, ord->status={0}", base::enum2index(ord->status));
                    }
                    break;
                }
                case ORDER_STATUS_TYPE::PENDING_CANCEL: {
                    switch (ord->status) {
                        case ORDER_STATUS_TYPE::CANCELLED: {
                            cached_order->mark_cancelled("order cancelled");
                            event::EventBus::publish_event(
                                    std::make_shared<event::Event<model::OrderPtr>>(event::ORDER_CANCELLATION_PASS,
                                                                                 cached_order));
                            cache_->remove_open_order(cached_order);
                            break;
                        }
                        case ORDER_STATUS_TYPE::FILLED: {
                            cached_order->status = ord->status;
                            event::EventBus::publish_event(
                                    std::make_shared<event::Event<model::OrderPtr>>(event::ORDER_UNSOLICITED_UPDATE,
                                                                                 cached_order));
                            cache_->remove_open_order(cached_order);
                            break;
                        }
                        default:
                            log_critical("unknown case on cached_order->status=ORDER_STATUS_TYPE::PENDING_CANCEL, ord->status={0}", base::enum2index(ord->status));
                    }
                    break;
                }
                default:
                    log_critical("unknown case, cached_order->status={0}", base::enum2index(cached_order->status));
            }
//            log_trace("on_order---processed cached_order: {0}", cached_order->to_string());
        }


        void CtpTradeGateway::on_trade(std::shared_ptr<model::Trade> trade) {
            sm_->guard();
            log_debug("成交回报: {0}", trade->to_string());
            if (Env::instance()->universe.count(trade->order_book_id) == 0) {
                auto &snapshot = Env::instance()->snapshot;
                if (Env::instance()->snapshot.count(trade->order_book_id) == 0) {
                    auto tick = std::make_shared<model::Tick>(trade->last_price);
                    snapshot[trade->order_book_id] = tick;
                }
/*                std::vector<std::string> tmp_vec;
                tmp_vec.emplace_back(trade->order_book_id);
                auto event = std::make_shared<event::Event<std::vector<std::string> &>>(event::POST_UNIVERSE_CHANGED, tmp_vec);
                event::EventBus::publish_event(event);*/
            }
            if (data_update_date_ != base::strftimep(system_clock::now(), "%Y%m%d")) {
                cache_->cache_trade(trade);
            } else {
                auto account = Env::instance()->portfolio->account();
                auto &backward_trade_set = account->backward_trade_set();
                if (backward_trade_set.find(trade->trade_id) != backward_trade_set.cend()) {
                    return;
                }
                auto order = cache_->get_cached_order(trade);
                trade->position_effect = order->position_effect;
                trade->frozen_price = order->price;
                auto comm = calc_commission(trade, order->position_effect);
//                log_trace("on_trade----order: {0}", order->to_string());
//                log_trace("on_trade----trade: {0}", trade->to_string());
                trade->commission = comm;
                order->fill(trade);
                auto trade_tuple = std::make_tuple<model::TradePtr, model::OrderPtr>(std::move(trade), std::move(order));

                event::EventBus::publish_event(
                        std::make_shared<event::Event<model::TradeTuple &>>(event::TRADE, trade_tuple));

            }
        }

        double CtpTradeGateway::calc_commission(std::shared_ptr<model::Trade> trade, const trade::POSITION_EFFECT_TYPE &poisition_effect) {
            auto &order_book_id = trade->order_book_id;
            auto &spec = (*cache_->future_info_)[order_book_id]->speculation;
            double comm = 0;
            if (spec.commission_type == COMMISSION_TYPE::BY_MONEY) {
                auto contract_multiplier = (*cache_->ins_)[order_book_id]->contract_multiplier;
                if (poisition_effect == POSITION_EFFECT_TYPE::OPEN) {
                    comm += trade->last_price * trade->last_quantity * contract_multiplier * spec.open_commission_ratio;
                } else if (poisition_effect == POSITION_EFFECT_TYPE::CLOSE) {
                    comm += trade->last_price * trade->last_quantity * contract_multiplier * spec.close_commission_ratio;
                } else if (poisition_effect == POSITION_EFFECT_TYPE::CLOSE_TODAY) {
                    comm += trade->last_price * trade->last_quantity * contract_multiplier * spec.close_commission_today_ratio;
                }
            } else {
                if (poisition_effect == POSITION_EFFECT_TYPE::OPEN) {
                    comm += trade->last_quantity * spec.open_commission_ratio;
                } else if (poisition_effect == POSITION_EFFECT_TYPE::CLOSE) {
                    comm += trade->last_quantity * spec.close_commission_ratio;
                } else if (poisition_effect == POSITION_EFFECT_TYPE::CLOSE_TODAY) {
                    comm += trade->last_quantity * spec.close_commission_today_ratio;
                }

            }
            return comm;
        }

        std::string CtpTradeGateway::get_trading_date() {
            return td_api_->getTradingDay();
        }

        void CtpTradeGateway::confirm_settlement_info() {
            for (auto i = 0; i < retry_times_; ++i) {
                try {
                    td_api_->reqSettlementInfoConfirm();
                    return;
                } catch (base::ReqException &ex) {
                } catch (base::RspException &ex) {
                }
                sleep(static_cast<unsigned >(retry_interval_ * (i + 1)));
            }
            MY_THROW(base::BaseException, "confirm_settlement_info超过次数");
        }

        void CtpTradeGateway::qry_instrument() {
            for (auto i = 0; i < retry_times_; ++i) {
                try {
                    auto ins_cache = td_api_->qry_instrument();
                    cache_->cache_ins(ins_cache);

/*                    if (Env::instance()->config.run_type == RUN_TYPE::COLLECT) {
                        std::vector<std::string> tmp_subs;
                        std::string pattern = "^.+$";
                        if (!collect_pattern_.empty()) {
                            pattern = "^(" + collect_pattern_ + ")\\d*$";
                        }
                        log_info("subscribed pattern: {0}", pattern);
                        std::regex re(pattern);
                        for (auto &ins : *ins_cache) {
                            if (std::regex_match(ins.first, re)) {
                                tmp_subs.push_back(ins.first);
                            }
                        }
                        if (!tmp_subs.empty()) {
                            auto event = std::make_shared<event::Event<std::vector<std::string> &>>(
                                    event::POST_UNIVERSE_CHANGED, tmp_subs);
                            event::EventBus::publish_event(event);
                        }
                    }*/
                    return;
                } catch (base::ReqException &ex) {
                } catch (base::RspException &ex) {
                }
                sleep(static_cast<unsigned>(retry_interval_ * (i + 1)));
            }
            MY_THROW(base::BaseException, "qry_instrument超过次数");
        }

        void CtpTradeGateway::qry_account() {
            for (auto i = 0; i < retry_times_; ++i) {
                try {
                    auto account = td_api_->qry_account();
                    cache_->cache_account(account);
                    return;
                } catch (base::ReqException &ex) {
                } catch (base::RspException &ex) {
                }
                sleep(static_cast<unsigned >(retry_interval_ * (i + 1)));
            }
            MY_THROW(base::BaseException, "qry_account超过次数");
        }

        void CtpTradeGateway::qry_position() {
            for (auto i = 0; i < retry_times_; ++i) {
                try {
                    auto pos_cache = td_api_->qry_position();
                    cache_->cache_position(pos_cache);
                    return;
                } catch (base::ReqException &ex) {
                } catch (base::RspException &ex) {
                }
                sleep(static_cast<unsigned >(retry_interval_ * (i + 1)));
            }
            MY_THROW(base::BaseException, "qry_position超过次数");
        }

        void CtpTradeGateway::qry_order() {
            for (auto i = 0; i < retry_times_; ++i) {
                try {
                    model::OrderMapPtr order_cache = td_api_->qry_order();
                    if (order_cache) {
                        for (auto &item : *order_cache) {
                            auto order = cache_->get_cached_order(item.second);
                            if (order->status == trade::ORDER_STATUS_TYPE::ACTIVE) {
                                cache_->cache_open_order(order);
                            }
                        }
                    }
                    return;
                } catch (base::ReqException &ex) {
                } catch (base::RspException &ex) {
                }
                sleep(static_cast<unsigned >(retry_interval_ * (i + 1)));
            }
            MY_THROW(base::BaseException, "qry_order超过次数");
        }

        model::CommissionMapPtr CtpTradeGateway::qry_commission(const std::string &instrument_id) {
            for (auto i = 0; i < retry_times_; ++i) {
                try {
                    auto commission_cache = td_api_->qry_commission(instrument_id);
//                    for (auto &item : *commission_cache) {
//                        cache_->cache_commission(global::make_order_book_id(instrument_id), commission);
//                    }
                    return commission_cache;
                } catch (base::ReqException &ex) {
                } catch (base::RspException &ex) {
                }
                sleep(static_cast<unsigned >(retry_interval_ * (i + 1)));
            }
            MY_THROW(base::BaseException, "qry_commission超过次数");
        }

        void CtpTradeGateway::qry_commission() {
            using namespace base::redis;
            using namespace service;
            FutureInfoService fis{};
            auto trading_date = get_trading_date();
            auto r = RedisFactory::get_redis();
/*            std::cout << "check_commission_date: " << fis.check_commission_date(r, trading_date) << std::endl;
            exit(1);*/
            std::string pattern = "^\\D+\\d{4}$";
            std::regex re(pattern);
            if (fis.check_update_commission_date(*r, trading_date)) {
                std::unordered_map<std::string, std::shared_ptr<model::Commission>> tmp_comm_map;
                auto commission_cache = qry_commission("");//qry commission which has position
                for (auto &kv : *commission_cache) {
                    tmp_comm_map[kv.first] = kv.second;
                }
                for (auto &item : *(cache_->ins_)) {
                    if (!std::regex_match(item.first, re)) continue;
                    auto iter1 = tmp_comm_map.find(item.second->underlying_symbol);
                    auto iter2 = Env::instance()->universe.find(item.first);
                    if (iter1 != tmp_comm_map.end() && iter2 == Env::instance()->universe.end()) {
                        cache_->cache_commission(item.first, iter1->second);
                        continue;
                    }
                    auto commission_cache = qry_commission(item.second->instrument_id);
                    for (auto &kv : *commission_cache) {
                        cache_->cache_commission(item.first, kv.second);
                        tmp_comm_map[kv.first] = kv.second;
                    }
//                    if (item.first == "RB1805") {
//                        log_info("order_book_id={0}, underlying_symbol", commission->order_book_id);
//                        exit(1);
//                    }
                }
                r = RedisFactory::get_redis();
                fis.add_commissions(*r, tmp_comm_map);
                fis.set_commission_date(*r, trading_date);
            } else {
                auto tmp_comm_map1 = fis.get_commissions(*r);
                for (auto &item : *(cache_->ins_)) {
                    if (!std::regex_match(item.first, re)) continue;
                    if (tmp_comm_map1.count(item.first)) {
                        cache_->cache_commission(item.first, tmp_comm_map1[item.first]);
//                        log_info("find order_book_id {0}", item.first);
                    } else {
                        cache_->cache_commission(item.first, tmp_comm_map1[item.second->underlying_symbol]);
//                        log_info("not find order_book_id {0}", item.first);
                    }
                }
//                log_info("RB1805's commission rate: {}", cache_->future_info_->at("RB1805")->speculation.open_commission_ratio);
            }
        }

        void CtpTradeGateway::wait_for(int timeout) {
            std::unique_lock<std::mutex> lock(mutex_);
            if (timeout == 0) {
                connect_cond_.wait(lock);
            } else {
                std::chrono::seconds sec(timeout);
                connect_cond_.wait_for(lock, sec);
            }
        }

        void CtpTradeGateway::notify() {
            connect_cond_.notify_all();
        }

/*        void CtpTradeGateway::set_collect_pattern(const std::string &collect_pattern) {
            collect_pattern_ = collect_pattern;
        }

        const std::string &CtpTradeGateway::get_collect_pattern() const {
            return collect_pattern_;
        }*/

    } /* ctp */

} /* iqt */ 

