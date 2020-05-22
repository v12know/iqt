#ifndef _IQT_SERVICE_STGY_SERVICE_HPP_
#define _IQT_SERVICE_STGY_SERVICE_HPP_

#include "model/stgy.h"
//#include "model/rpt.h"

#include "dao/stgy_dao.hpp"

//#include "base/redis.h"
#include "base/log.h"


namespace iqt {
    namespace service {
//    using namespace base::redis;
        using namespace model;
        using namespace dao;

        class StgyService {
        public:
            void load_stgy(cpp_redis::client &r, Stgy &stgy) {

                stgy_dao_.get_rpt_key(r, stgy);

                log_info("load_stgy finish, stgy_id={0}", stgy.stgy_id);
            }

            void clear_handling_stgy(cpp_redis::client &r, const Stgy &stgy) {
                std::string stgy_type_key = model::stgy::ACCOUNT + stgy.account_id + ":" + stgy.stgy_type + suffix::ING;
                stgy_dao_.del(r, {stgy_type_key});
            }

        private:
            StgyDao stgy_dao_;
        };

    } /* service */
} /* iqt */
#endif /* end of include guard: _IQT_SERVICE_STGY_SERVICE_HPP_ */
