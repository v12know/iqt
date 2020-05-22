#ifndef _IQT_MODEL_POSITION_H_
#define _IQT_MODEL_POSITION_H_

#include <string>
#include <unordered_map>
#include <memory>
#include <sstream>

#include "model/instrument.h"
#include "ctp/ThostFtdcUserApiStruct.h"

namespace iqt {
    namespace model {

        struct Position {
            std::string order_book_id;
            int buy_quantity {0};
            int buy_old_quantity {0};
            int buy_today_quantity {0};
            double buy_transaction_cost {0.};
            double buy_realized_pnl {0.};
            double buy_open_cost {0.};
            double buy_avg_open_price {0.};


            int sell_quantity {0};
            int sell_old_quantity {0};
            int sell_today_quantity {0};
            double sell_transaction_cost {0.};
            double sell_realized_pnl {0.};
            double sell_open_cost {0.};
            double sell_avg_open_price {0.};

            int contract_multiplier {1};
            double pre_settlement_price {0.};


            Position() = default;
            Position(CThostFtdcInvestorPositionField *data, model::Instrument *ins = nullptr) {
                if (ins) {
                     contract_multiplier = ins->contract_multiplier;
                }
                update(data);
            }

            std::string to_string() {
                std::ostringstream oss;
                oss << "order_book_id=" << order_book_id << ", buy_quantity=" << buy_quantity << ", buy_old_quantity=" << buy_old_quantity << ", buy_today_quantity=" << buy_today_quantity
                    << ", buy_transaction_cost=" << buy_transaction_cost << ", buy_realized_pnl=" << buy_realized_pnl
                    << ", buy_open_cost=" << buy_open_cost << ", buy_avg_open_price=" << buy_avg_open_price
                    << ", sell_quantity=" << sell_quantity << ", sell_old_quantity=" << sell_old_quantity << ", sell_today_quantity=" << sell_today_quantity
                    << ", sell_transaction_cost=" << sell_transaction_cost << ", sell_realized_pnl=" << sell_realized_pnl
                    << ", sell_open_cost=" << sell_open_cost << ", sell_avg_open_price=" << sell_avg_open_price
                    << ", contract_multiplier=" << contract_multiplier << ", pre_settlement_price=" << pre_settlement_price << std::endl;
                return oss.str();
            }

            void update(CThostFtdcInvestorPositionField *data) {
                order_book_id = global::make_order_book_id(data->InstrumentID);
                if (data->PosiDirection == THOST_FTDC_PD_Net || data->PosiDirection == THOST_FTDC_PD_Long) {
                    buy_quantity += data->Position;
                    buy_today_quantity += data->TodayPosition;
                    buy_old_quantity = buy_quantity - buy_today_quantity;
                    buy_transaction_cost += data->Commission;
                    buy_realized_pnl += data->CloseProfit;
                    buy_open_cost += data->OpenCost;
                    if (buy_quantity) {
                        buy_avg_open_price = buy_open_cost / (buy_quantity * contract_multiplier);
                    }
                } else {
                    sell_quantity += data->Position;
                    sell_today_quantity += data->TodayPosition;
                    sell_old_quantity = sell_quantity - sell_today_quantity;
                    sell_transaction_cost += data->Commission;
                    sell_realized_pnl += data->CloseProfit;
                    sell_open_cost += data->OpenCost;
                    if (sell_quantity) {
                        sell_avg_open_price = sell_open_cost / (sell_quantity * contract_multiplier);
                    }
                }
                if (data->PreSettlementPrice) {
                    pre_settlement_price = data->PreSettlementPrice;
                }
            }

            Position(const Position &pos) = default;
            Position(Position &&pos) = default;
        };

        typedef std::unordered_map<std::string, std::shared_ptr<model::Position>> PositionMap;
        typedef std::shared_ptr<model::PositionMap> PositionMapPtr;

    } /* model  */

} /* iqt */
#endif /* end of include guard: _IQT_MODEL_POSITION_H_ */
