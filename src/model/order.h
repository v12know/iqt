#ifndef _IQT_MODEL_ORDER_H_
#define _IQT_MODEL_ORDER_H_

#include <memory>
#include <string>
#include <cassert>
#include <sstream>

#include "ctp/ThostFtdcUserApiStruct.h"
#include "const.hpp"
#include "model/trade.h"

namespace iqt {
    namespace model {
        namespace order {
            static const std::string ORDER = "order:";
            static const std::string CREATE_TIME = "create_time";
            static const std::string TRADING_DATE = "trading_date";
            static const std::string ORDER_BOOK_ID = "order_book_id";
/*            static const std::string FRONT_ID = "front_id";
            static const std::string SESSION_ID = "session_id";
            static const std::string ORDER_ID = "order_id";*/
            static const std::string SIDE = "side";
            static const std::string POSITION_EFFECT = "position_effect";
            static const std::string STATUS = "status";
            static const std::string AVG_PRICE = "avg_price";
            static const std::string FILLED_QUANTITY = "filled_quantity";
            static const std::string PRICE = "price";
            static const std::string QUANTITY = "quantity";
            static const std::string TYPE = "type";
            static const std::string TRANSACTION_COST = "transaction_cost";

            static const std::string ORDER_KEY = "order_key";
            static const std::string ORDER_SYS_KEY = "order_sys_key";
            static const std::string GROUP_KEY = "group_key";
            static const std::string RPT_KEY = "rpt_key";

        }
        struct Trade;


        struct Order {
            std::string create_time;
            std::string trading_date;
            std::string order_book_id;
            int front_id;
            int session_id;
            std::string order_id;
            std::string order_sys_id;
            std::string exchange_id;
            trade::SIDE_TYPE side;
            trade::POSITION_EFFECT_TYPE position_effect;
            trade::ORDER_STATUS_TYPE status;
            double avg_price = 0;
            int filled_quantity = 0;
            int trade_filled_quantity = 0;
            double price = 0;
            int quantity;
            trade::ORDER_TYPE type;
            int min_quantity = 1;
            double transaction_cost = 0;

            std::string order_key;
            std::string group_key;
            std::string rpt_key;
            std::string order_sys_key = "";

            std::string message;

            void set_order_key(int front_id_, int session_id_, const std::string &order_id_) {
                front_id = front_id_;
                session_id = session_id_;
                order_id = order_id_;
                order_key = order::ORDER + std::to_string(front_id) + ":" + std::to_string(session_id) + ":" + order_id;
            }
            void set_order_sys_key(const std::string &exchange_id_, const std::string &order_sys_id_) {
                exchange_id = exchange_id_;
                order_sys_id = order_sys_id_;
                order_sys_key = order::ORDER + exchange_id + ":" + order_sys_id;
            }

            Order() = default;

            Order(const Order &order) = default;

            Order(Order &&order) noexcept = default;

            Order(const std::string &_order_book_id, int _quantity, const trade::SIDE_TYPE &_side, const trade::ORDER_TYPE &_type,
                  const trade::POSITION_EFFECT_TYPE &_position_effect, double _price = 0, const std::string &_rpt_key = "", int _min_quantity = 1) :
                    order_book_id(_order_book_id), side(_side), position_effect(_position_effect), status(trade::ORDER_STATUS_TYPE::PENDING_NEW),
                    avg_price(_price), price(_price), quantity(_quantity), type(_type), min_quantity(_min_quantity), rpt_key(_rpt_key) {}

            Order(const CThostFtdcOrderField *pOrder) :
//                    date(pOrder->InsertDate), time(pOrder->InsertTime), trading_date(pOrder->TradingDay),
                    order_book_id(global::make_order_book_id(pOrder->InstrumentID)), front_id(pOrder->FrontID), session_id(pOrder->SessionID),
                    order_id(pOrder->OrderRef), order_sys_id(pOrder->OrderSysID), exchange_id(pOrder->ExchangeID),
                    side(map_side_from_ctp(pOrder->Direction)),
                    position_effect(map_position_effect_from_ctp(pOrder->ExchangeID, pOrder->CombOffsetFlag[0])),
                    status(map_order_status_from_ctp(pOrder->OrderStatus)), avg_price(pOrder->LimitPrice),
                    filled_quantity(pOrder->VolumeTraded),
                    price(pOrder->LimitPrice), quantity(pOrder->VolumeTotalOriginal),
                    order_key(order::ORDER + std::to_string(front_id) + ":" + std::to_string(session_id) + ":" + order_id),
                    order_sys_key(order::ORDER + pOrder->ExchangeID + ":" + pOrder->OrderSysID) {}

            Order(const CThostFtdcInputOrderField *pInputOrder, int _front_id, int _session_id) :
                    order_book_id(global::make_order_book_id(pInputOrder->InstrumentID)), front_id(_front_id), session_id(_session_id),
                    order_id(pInputOrder->OrderRef), order_sys_id("-1"), exchange_id(pInputOrder->ExchangeID),
                    side(map_side_from_ctp(pInputOrder->Direction)),
                    position_effect(map_position_effect_from_ctp(pInputOrder->ExchangeID, pInputOrder->CombOffsetFlag[0])),
                    status(trade::ORDER_STATUS_TYPE::REJECTED), avg_price(pInputOrder->LimitPrice), filled_quantity(0),
                    price(pInputOrder->LimitPrice), quantity(pInputOrder->VolumeTotalOriginal),
                    order_key(order::ORDER + std::to_string(front_id) + ":" + std::to_string(session_id) + ":" + order_id) {}

            std::string to_string() {
                std::ostringstream oss;
                oss << "create_time=" << create_time << ", trading_date=" << trading_date << ", front_id=" << front_id
                    << ", session_id=" << session_id << ", order_id=" << order_id << ", order_sys_id=" << order_sys_id
                    << ", exchange_id=" << exchange_id << ", side=" << trade::s_side_type.enum2literal(side) << ", position_effect=" << trade::s_position_effect_type.enum2literal(position_effect)
                    << ", status=" << trade::s_order_status_type.enum2literal(status) << ", avg_price=" << avg_price << ", trade_filled_quantity=" << trade_filled_quantity << ", filled_quantity=" << filled_quantity
                    << ", price=" << price << ", quantity=" << quantity << ", order_book_id=" << order_book_id << std::endl;
                return oss.str();
            }

            void fill(std::shared_ptr<model::Trade> trade) {
                assert(trade_filled_quantity + trade->last_quantity <= quantity);
                auto new_quan = trade_filled_quantity + trade->last_quantity;
                avg_price = (avg_price * trade_filled_quantity + trade->last_price * trade->last_quantity) / new_quan;
                transaction_cost += trade->commission + trade->tax;
                trade_filled_quantity = new_quan;
                if (quantity - trade_filled_quantity == 0) {
                    status = trade::ORDER_STATUS_TYPE::FILLED;
                }
            }

            bool is_final() {
                return status != trade::ORDER_STATUS_TYPE::PENDING_NEW && status != trade::ORDER_STATUS_TYPE::ACTIVE && status != trade::ORDER_STATUS_TYPE::PENDING_CANCEL;
            }

            void active() {
                status = trade::ORDER_STATUS_TYPE::ACTIVE;
            }

            void set_pending_cancel() {
                if (!is_final()) {
                    status = trade::ORDER_STATUS_TYPE::PENDING_CANCEL;
                }
            }

            void mark_rejected(const std::string &rejected_reason) {
                if (!is_final()) {
                    message = rejected_reason;
                    status = trade::ORDER_STATUS_TYPE::REJECTED;
                }
            }

            void mark_cancelled(const std::string &cancelled_reason) {
                if (!is_final()) {
                    message = cancelled_reason;
                    status = trade::ORDER_STATUS_TYPE::CANCELLED;
                }
            }

            static trade::SIDE_TYPE map_side_from_ctp(char direction) {
                static std::unordered_map<char, trade::SIDE_TYPE> s_side_map = {
                        {THOST_FTDC_D_Buy,  trade::SIDE_TYPE::BUY},
                        {THOST_FTDC_D_Sell, trade::SIDE_TYPE::SELL}
                };
                return s_side_map[direction];
            }

            char map_side_to_ctp() {
                static std::unordered_map<trade::SIDE_TYPE, char, base::EnumClassHash> s_side_map = {
                        {trade::SIDE_TYPE::BUY,  THOST_FTDC_D_Buy},
                        {trade::SIDE_TYPE::SELL, THOST_FTDC_D_Sell}
                };
                return s_side_map[side];
            }

            static trade::POSITION_EFFECT_TYPE map_position_effect_from_ctp(const char *exchangeID, char offsetFlag) {
                if (trade::EXCHANGE_ID::SHFE.compare(exchangeID) == 0) {
                    if (offsetFlag == THOST_FTDC_OFEN_Open) {
                        return trade::POSITION_EFFECT_TYPE::OPEN;
                    } else if (offsetFlag == THOST_FTDC_OFEN_CloseToday) {
                        return trade::POSITION_EFFECT_TYPE::CLOSE_TODAY;
                    } else {
                        return trade::POSITION_EFFECT_TYPE::CLOSE;
                    }
                } else {
                    if (offsetFlag == THOST_FTDC_OFEN_Open) {
                        return trade::POSITION_EFFECT_TYPE::OPEN;
                    } else {
                        return trade::POSITION_EFFECT_TYPE::CLOSE;
                    }
                }
            }

            char map_position_effect_to_ctp() {
                static std::unordered_map<trade::POSITION_EFFECT_TYPE, char, base::EnumClassHash> position_effect_map = {
                        {trade::POSITION_EFFECT_TYPE::OPEN,        THOST_FTDC_OFEN_Open},
                        {trade::POSITION_EFFECT_TYPE::CLOSE,       THOST_FTDC_OFEN_Close},
                        {trade::POSITION_EFFECT_TYPE::CLOSE_TODAY, THOST_FTDC_OFEN_CloseToday},
                };
                return position_effect_map[position_effect];
            }

//            static base::EnumWrap<trade::POSITION_EFFECT_TYPE, char> s_ctp_position_effect_type{THOST_FTDC_OFEN_Open, THOST_FTDC_OFEN_Close, THOST_FTDC_OFEN_CloseToday};

            static trade::ORDER_STATUS_TYPE map_order_status_from_ctp(char orderStatus) {
                if (orderStatus == THOST_FTDC_OST_PartTradedQueueing || orderStatus == THOST_FTDC_OST_NoTradeQueueing) {
                    return trade::ORDER_STATUS_TYPE::ACTIVE;
                } else if (orderStatus == THOST_FTDC_OST_AllTraded) {
                    return trade::ORDER_STATUS_TYPE::FILLED;
                } else if (orderStatus == THOST_FTDC_OST_Canceled) {
                    return trade::ORDER_STATUS_TYPE::CANCELLED;
                } else {
                    return trade::ORDER_STATUS_TYPE::ACTIVE;
                }
            }

            char map_order_price_type_to_ctp() {
                static std::unordered_map<trade::ORDER_TYPE, char, base::EnumClassHash> s_order_price_type_map = {
                        {trade::ORDER_TYPE::MARKET, THOST_FTDC_OPT_AnyPrice},
                        {trade::ORDER_TYPE::LIMIT,  THOST_FTDC_OPT_LimitPrice},
                        {trade::ORDER_TYPE::FAK,    THOST_FTDC_OPT_LimitPrice},
                        {trade::ORDER_TYPE::FOK,    THOST_FTDC_OPT_LimitPrice}
                };
                return s_order_price_type_map[type];
            }

            char map_time_condition_type_to_ctp() {
                static std::unordered_map<trade::ORDER_TYPE, char, base::EnumClassHash> s_time_condition_type_map = {
                        {trade::ORDER_TYPE::MARKET, THOST_FTDC_TC_GFD},
                        {trade::ORDER_TYPE::LIMIT,  THOST_FTDC_TC_GFD},
                        {trade::ORDER_TYPE::FAK,    THOST_FTDC_TC_IOC},
                        {trade::ORDER_TYPE::FOK,    THOST_FTDC_TC_IOC}
                };
                return s_time_condition_type_map[type];
            }

            char map_volume_condition_type_to_ctp() {
                static std::unordered_map<trade::ORDER_TYPE, char, base::EnumClassHash> s_volume_condition_type_map = {
                        {trade::ORDER_TYPE::MARKET, THOST_FTDC_VC_AV},
                        {trade::ORDER_TYPE::LIMIT,  THOST_FTDC_VC_AV},
                        {trade::ORDER_TYPE::FAK,    THOST_FTDC_VC_AV},
                        {trade::ORDER_TYPE::FOK,    THOST_FTDC_VC_CV}
                };
                if (min_quantity >= 2) {
                    return THOST_FTDC_VC_MV;
                }
                return s_volume_condition_type_map[type];
            }
        };

        typedef std::shared_ptr<model::Order> OrderPtr;
        typedef std::unordered_map<std::string, std::shared_ptr<model::Order>> OrderMap;
        typedef std::shared_ptr<model::OrderMap> OrderMapPtr;
    } /* model  */

} /* iqt */
#endif /* end of include guard: _IQT_MODEL_ORDER_H_ */
