#ifndef _IQT_SERVICE_RPT_SERVICE_HPP_
#define _IQT_SERVICE_RPT_SERVICE_HPP_

#include "base/util.hpp"
#include "model/rpt.h"
#include "dao/rpt_dao.hpp"
//#include "base/redis.h"


namespace iqt {
    namespace service {
//    using namespace base::redis;
        using namespace model;
        using namespace dao;

        class RptService {
        public:
            void load_rpt(cpp_redis::client &r, Rpt &rpt) {
                if (rpt_dao_.exists(r, {rpt.rpt_key})) {
                    rpt_dao_.get_rpt(r, rpt);
                } else {
                    rpt.create_time = base::strftimep();
                    rpt_dao_.add_rpt(r, rpt);
                }

                log_info("load_rpt finish, rpt_key={0}", rpt.rpt_key);
            }

            std::string replace_handling_txn_key(cpp_redis::client &r, const std::string &rpt_txning_key,
                                                 const std::string &rpt_txn_key) {
                if (rpt_dao_.exists(r, {rpt_txning_key})) {

                    return rpt_dao_.get(r, rpt_txning_key);
                } else {
                    auto txn_key = txn::TXN + std::to_string(rpt_dao_.incr(r, txn::TXN));
                    rpt_dao_.set(r, rpt_txning_key, txn_key);
                    return txn_key;
                }
            }

            std::string get_handling_txn_key(cpp_redis::client &r, const std::string &rpt_key,
                                             const std::string &txn_key_field) {
                if (rpt_dao_.hexists(r, rpt_key, txn_key_field)) {
                    return rpt_dao_.hget(r, rpt_key, txn_key_field);
                } else {
                    auto txn_key = txn::TXN + std::to_string(rpt_dao_.incr(r, txn::TXN));
                    rpt_dao_.hset(r, rpt_key, txn_key_field, txn_key);
                    return txn_key;
                }
            }

            void update_rpt(cpp_redis::client &r, Rpt &rpt) {
                rpt_dao_.update_rpt(r, rpt);
            }

        private:
            RptDao rpt_dao_;
        };

    } /* service */
} /* iqt */
#endif /* end of include guard: _IQT_SERVICE_RPT_SERVICE_HPP_ */
