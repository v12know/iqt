#ifndef _IQT_MODEL_TRADE_H_
#define _IQT_MODEL_TRADE_H_

#include <string>
#include <list>
#include <unordered_map>
#include <memory>
#include <sstream>
#include <tuple>

#include "ctp/ThostFtdcUserApiStruct.h"
#include "const.hpp"
#include "global.h"
//#include "model/order.h"

namespace iqt {
    namespace model {
        static const std::string ORDER = "order:";
        struct Order;


        struct Trade {
            std::string date;
            std::string time;
            std::string trading_date;
            std::string order_book_id;
            std::string trade_id;
            std::string order_id;
            std::string order_sys_id;
            std::string exchange_id;
            trade::SIDE_TYPE side;
            trade::POSITION_EFFECT_TYPE position_effect;
            double last_price;
            double frozen_price = 0;
            int last_quantity;
            double commission;
            double tax = 0;
            std::string order_sys_key;

            Trade() = default;

            Trade(const Trade &trade) = default;

            Trade(Trade &&trade) noexcept = default;

            Trade(const CThostFtdcTradeField *pTrade) :
                    date(pTrade->TradeDate), time(pTrade->TradeTime), trading_date(pTrade->TradingDay),
                    order_book_id(global::make_order_book_id(pTrade->InstrumentID)), trade_id(pTrade->TradeID),
                    order_id(pTrade->OrderRef), order_sys_id(pTrade->OrderSysID), exchange_id(pTrade->ExchangeID),
                    side(map_side_from_ctp(pTrade->Direction)),
                    position_effect(map_position_effect_from_ctp(pTrade->ExchangeID, pTrade->OffsetFlag)),
                    last_price(pTrade->Price), last_quantity(pTrade->Volume),
                    order_sys_key(ORDER + pTrade->ExchangeID + ":" + pTrade->OrderSysID) {}

            std::string to_string() {
                std::ostringstream oss;
                oss << "date=" << date << ", time=" << time << ", trading_date=" << trading_date << ", order_id=" << order_id
                    << ", order_book_id=" << order_book_id << ", order_sys_id=" << order_sys_id
                    << ", exchange_id=" << exchange_id << ", side=" << trade::s_side_type.enum2literal(side) << ", position_effect=" << trade::s_position_effect_type.enum2literal(position_effect)
                    << ", last_price=" << last_price << ", last_quantity=" << last_quantity << ", transaction_cost=" << transaction_cost() << std::endl;
                return oss.str();
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

            double transaction_cost() {
                return tax + commission;
            }

        };
        typedef std::shared_ptr<model::Trade> TradePtr;
        typedef std::shared_ptr<model::Order> OrderPtr;
        typedef std::tuple<TradePtr, OrderPtr> TradeTuple;
        typedef std::unordered_map<std::string, std::list<std::shared_ptr<model::Trade>>> TradeListMap;
        typedef std::shared_ptr<model::TradeListMap> TradeListMapPtr;

    } /* model  */

} /* iqt */
#endif /* end of include guard: _IQT_MODEL_TRADE_H_ */
