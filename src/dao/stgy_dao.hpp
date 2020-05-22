#ifndef _IQT_DAO_STGY_DAO_HPP_
#define _IQT_DAO_STGY_DAO_HPP_

#include "dao/base_dao.hpp"
#include "model/stgy.h"


namespace iqt {
namespace dao {
    using namespace base::redis;
    using namespace model;
    class StgyDao : public BaseDao {
    public:
        void get_rpt_key(cpp_redis::client &r, Stgy &stgy) {
            std::string stgy_type_key = model::stgy::ACCOUNT + stgy.account_id + ":" + stgy.stgy_type;
            std::string stgy_type_ing_key = stgy_type_key + suffix::ING;
            std::string stgy_member = stgy.stgy_id + ":" + stgy.version;

            auto zscore = r.zscore(stgy_type_key, stgy_member);
            r.sync_commit();
            auto reply = zscore.get();
            std::string rpt_id;
            if (reply.is_null()) {
                rpt_id = std::to_string(incr(r, rpt::RPT));
                r.zadd(stgy_type_key, {}, {
                        {rpt_id, stgy_member}
                });
            } else {
                rpt_id = reply.as_string();
            }
            r.zadd(stgy_type_ing_key, {}, {
                    {rpt_id, stgy_member}
            });
            stgy.rpt_key = rpt::RPT + rpt_id;
        }
    };
} /* dao  */

} /* iqt */

#endif /* end of include guard: _IQT_DAO_STGY_DAO_HPP_ */
