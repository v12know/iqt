#ifndef _IQT_MODEL_ACCOUNT_H_
#define _IQT_MODEL_ACCOUNT_H_

#include <string>

#include "ctp/ThostFtdcUserApiStruct.h"

namespace iqt {
    namespace model {
        struct Account {
            //代理商id
            std::string broker_id;
            //用户id
            std::string user_id;
            //静态权益
            double pre_balance;
            //动态权益
            double balance;
            //平仓盈亏
            double close_profit;
            //持仓盈亏
            double position_profit;
            //当前占用保证金
            double curr_margin;
            //手续费
            double commission;
            //可用资金
            double available;

            Account(CThostFtdcTradingAccountField *data) :
                    broker_id(data->BrokerID), user_id(data->AccountID),
                    pre_balance(data->PreBalance),
                    balance(data->Balance), close_profit(data->CloseProfit),
                    position_profit(data->PositionProfit),
                    curr_margin(data->CurrMargin),
                    commission(data->Commission), available(data->Available) {}
        };

    } /* model  */

} /* iqt */
#endif /* end of include guard: _IQT_MODEL_ACCOUNT_H_ */
