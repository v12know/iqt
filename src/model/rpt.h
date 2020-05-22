#ifndef _IQT_MODEL_RPT_H_
#define _IQT_MODEL_RPT_H_

#include <vector>

namespace iqt {
    namespace model {
        namespace rpt {
            static const std::string RPT = "rpt:";
            static const std::string STGY_ID = "stgy_id";
            static const std::string CREATE_TIME = "create_time";
            static const std::string LONG_TXN_KEY = "long_txn_key";
            static const std::string SHORT_TXN_KEY = "short_txn_key";

            static const std::string REALIZED_PNL = "realized_pnl";
            static const std::string HOLDING_PNL = "holding_pnl";
            static const std::string TRANSACTION_COST = "transaction_cost";
            static const std::string LOSS_COUNT = "loss_count";
        }
        struct Rpt {
            std::string stgy_id;
            std::string rpt_key;
            std::string create_time;
            std::string long_txn_key;
            std::string short_txn_key;
            double realized_pnl = 0;
            double holding_pnl = 0;
            double transaction_cost = 0;


            int loss_count = 0;
        };

    } /* model  */

} /* iqt */
#endif /* end of include guard: _IQT_MODEL_RPT_H_ */
