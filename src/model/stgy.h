#ifndef _IQT_MODEL_STGY_H_
#define _IQT_MODEL_STGY_H_

#include <string>

namespace iqt {
    namespace model {
        namespace stgy {
            static const std::string STGY = "stgy:";
            static const std::string ACCOUNT = "account:";
//            static const std::string STGY_ID = "stgy_id";
//            static const std::string RPT_ID = "rpt_id";
        }

        struct Stgy {
            std::string stgy_type;//策略类型
            std::string stgy_id;//在配置文件中的唯一标识策略stgy_id
            std::string version;//版本号
            std::string account_id;//帐号id，${broker_id}:${user_id}
            std::string rpt_key;
        };
    } /* model  */

} /* iqt */
#endif /* end of include guard: _IQT_MODEL_STGY_H_ */
