#ifndef _IQT_DAO_RPT_DAO_HPP_
#define _IQT_DAO_RPT_DAO_HPP_

#include "dao/base_dao.hpp"
#include "model/rpt.h"

namespace iqt {
    namespace dao {
//    using namespace base::redis;
        using namespace model;

        class RptDao : public BaseDao {
        public:
            void add_rpt(cpp_redis::client &r, Rpt &rpt) {
                r.hmset(rpt.rpt_key, {
                        {rpt::STGY_ID, rpt.stgy_id},
                        {rpt::REALIZED_PNL, base::to_string(rpt.realized_pnl)},
//                        {rpt::HOLDING_PNL, base::to_string(rpt.holding_pnl)},
                        {rpt::TRANSACTION_COST, base::to_string(rpt.transaction_cost)},
                        {rpt::LOSS_COUNT, std::to_string(rpt.loss_count)},
                        {rpt::CREATE_TIME, rpt.create_time}
                });
            }

            void get_rpt(cpp_redis::client &r, Rpt &rpt) {
                auto hmget = r.hmget(rpt.rpt_key, fields_);
                r.sync_commit();
                auto array = hmget.get().as_array();
                rpt.create_time = array[0].as_string();
                if (!array[1].is_null())//TODO delete
                    rpt.realized_pnl = std::stod(array[1].as_string());
                if (!array[2].is_null())//TODO delete
                    rpt.transaction_cost = std::stod(array[2].as_string());
                if (!array[3].is_null())//TODO delete
                    rpt.loss_count = std::stoi(array[3].as_string());
            }

            void update_rpt(cpp_redis::client &r, Rpt &rpt) {
                r.hmset(rpt.rpt_key, {
                        {rpt::REALIZED_PNL, base::to_string(rpt.realized_pnl)},
                        {rpt::HOLDING_PNL, base::to_string(rpt.holding_pnl)},
                        {rpt::TRANSACTION_COST, base::to_string(rpt.transaction_cost)},
                        {rpt::LOSS_COUNT, std::to_string(rpt.loss_count)}
                });
            }
        private:
            std::vector<std::string> fields_ = {rpt::CREATE_TIME, rpt::REALIZED_PNL, rpt::TRANSACTION_COST, rpt::LOSS_COUNT};
        };
    } /* dao  */

} /* iqt */

#endif /* end of include guard: _IQT_DAO_RPT_DAO_HPP_ */
