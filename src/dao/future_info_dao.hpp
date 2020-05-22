#ifndef _IQT_DAO_FUTURE_INFO_DAO_HPP_
#define _IQT_DAO_FUTURE_INFO_DAO_HPP_

#include "dao/base_dao.hpp"
#include "model/future_info.h"

namespace iqt {
    namespace dao {
//    using namespace base::redis;
        using namespace model;

        class FutureInfoDao : public BaseDao {
        public:
            void add_commissions(cpp_redis::client &r, const std::string &comm_key, const std::vector<std::string> &comm_vec) {
                if (exists(r, {comm_key})) {
                    del(r, {comm_key});
                }
                r.rpush(comm_key, comm_vec);
            }
            std::vector<std::string> get_commissions(cpp_redis::client &r, const std::string &comm_key) {
                auto lrange = r.lrange(comm_key, 0, -1);
                r.sync_commit();
                auto array = lrange.get().as_array();
                std::vector<std::string> comm_vec;
                for (auto &reply : array) {
                    comm_vec.push_back(reply.as_string());
                }
                return comm_vec;
            };
        private:
//            std::vector<std::string> fields_ = {rpt::CREATE_TIME, rpt::REALIZED_PNL};
        };
    } /* dao  */

} /* iqt */

#endif /* end of include guard: _IQT_DAO_FUTURE_INFO_DAO_HPP_ */
