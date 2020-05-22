#ifndef _IQT_EVENT_SOURCE_H_
#define _IQT_EVENT_SOURCE_H_

#include <string>
#include <memory>
#include <chrono>

#include "base/macro.hpp"


namespace cpp_redis {
    class client;
}
namespace base {
    class Timer;
}
namespace iqt {
    namespace dao {
        class BaseDao;
    }
    namespace event {
        static const std::string MIN_DATE = "19700101";
        static const std::string SETTLEMENT_TIME = "0320";
        static const std::string BEFORE_TRADING_DATE_KEY = "before_trading_date:";
        static const std::string AFTER_TRADING_DATE_KEY = "after_trading_date:";

        class TradeEventSource {
        public:
            ~TradeEventSource();

            static TradeEventSource *instance();

            void start();

            DISALLOW_COPY_AND_ASSIGN(TradeEventSource);
        private:
            TradeEventSource();

            bool check_before_trading(cpp_redis::client &r, const std::string &now_datetime);

            bool check_after_trading(cpp_redis::client &r, const std::string &now_datetime);

        private:
            base::Timer *timer_;
            dao::BaseDao *base_dao_;
            std::string before_trading_date_ = "";
            std::string after_trading_date_ = "";

        };

    } /* event */
} /* iqt */

#endif /* end of include guard: _IQT_EVENT_SOURCE_H_ */
