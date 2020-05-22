#ifndef _IQT_CONST_HPP_
#define _IQT_CONST_HPP_

#include <string>

#include "base/enum_wrap.hpp"
/**
 * @file var.h
 * @synopsis 集中存放常量
 * @author Carl, v12know@hotmail.com
 * @version 1.0.0
 * @date 2017-01-09
 */

namespace iqt {

    namespace trade {
//        namespace MARGIN_TYPE {
//            static const std::string BY_MONEY = "BY_MONEY";
//            static const std::string BY_VOLUME = "BY_VOLUME";
//        }
        enum class MARGIN_TYPE {
            BY_MONEY,
            BY_VOLUME
        };

        static base::EnumWrap<MARGIN_TYPE, std::string> s_margin_type({"BY_MONEY", "BY_VOLUME"});

//        namespace COMMISSION_TYPE {
//            static const std::string BY_MONEY = "BY_MONEY";
//            static const std::string BY_VOLUME = "BY_VOLUME";
//        }
        enum class COMMISSION_TYPE {
            BY_MONEY,
            BY_VOLUME
        };

        static base::EnumWrap<COMMISSION_TYPE, std::string> s_commission_type({"BY_MONEY", "BY_VOLUME"});

        namespace EXCHANGE_ID {
            static const std::string SHFE = "SHFE";
            static const std::string CZCE = "CZCE";
            static const std::string DCE = "DCE";
            static const std::string CFFEX = "CFFEX";
            static const std::string INE = "INE";
        }

/*        enum class EXCHANGE_ID {
            SHFE,
            CZCE,
            DCE,
            CFFEX,
            INE
        };

        static base::EnumWrap<EXCHANGE_ID, std::string> s_exchange_id({"SHFE", "CZCE", "DCE", "CFFEX", "INE"});*/

/*        namespace POSITION_EFFECT_TYPE {
            static const std::string OPEN = "OPEN";
            static const std::string CLOSE = "CLOSE";
            static const std::string CLOSE_TODAY = "CLOSE_TODAY";
        }*/
        enum class POSITION_EFFECT_TYPE {
            OPEN,
            CLOSE,
            CLOSE_TODAY
        };

        static base::EnumWrap<POSITION_EFFECT_TYPE, std::string> s_position_effect_type({"OPEN", "CLOSE", "CLOSE_TODAY"});

/*        namespace SIDE_TYPE {
            static const std::string BUY = "BUY";
            static const std::string SELL = "SELL";
        }*/
        enum class SIDE_TYPE {
            BUY,
            SELL,
        };

        static base::EnumWrap<SIDE_TYPE, std::string> s_side_type({"BUY", "SELL"});

/*        namespace ORDER_STATUS_TYPE {
            static const std::string PENDING_NEW = "PENDING_NEW";
            static const std::string ACTIVE = "ACTIVE";
            static const std::string FILLED = "FILLED";
            static const std::string REJECTED = "REJECTED";
            static const std::string PENDING_CANCEL = "PENDING_CANCEL";
            static const std::string CANCELLED = "CANCELLED";
        }*/
        enum class ORDER_STATUS_TYPE {
            PENDING_NEW,
            ACTIVE,
            FILLED,
            REJECTED,
            PENDING_CANCEL,
            CANCELLED
        };

        static base::EnumWrap<ORDER_STATUS_TYPE, std::string> s_order_status_type({"PENDING_NEW", "ACTIVE", "FILLED", "REJECTED", "PENDING_CANCEL", "CANCELLED"});

/*        namespace ORDER_TYPE {
            static const std::string MARKET = "MARKET";
            static const std::string LIMIT = "LIMIT";
            static const std::string FAK = "FAK";
            static const std::string FOK = "FOK";
        }*/
        enum class ORDER_TYPE {
            MARKET,
            LIMIT,
            FAK,
            FOK,
        };

        static base::EnumWrap<ORDER_TYPE, std::string> s_order_type({"MARKET", "LIMIT", "FAK", "FOK"});
    }

    namespace DAYS_CNT {

        static const int DAYS_A_YEAR = 365;
        static const int TRADING_DAYS_A_YEAR = 252;
    }

    namespace sign {
        static const int POS = 1;//for long
        static const int NEG = -1;//for short
    }


    namespace model {
        namespace suffix {
            static const std::string LONG = ":long";
            static const std::string SHORT = ":short";
            static const std::string TXN = ":txn";
            static const std::string ING = ":ing";
            static const std::string SLOT = ":slot";
            static const std::string COST = ":cost";
            static const std::string LOCK = ":lock";

            static const std::string OPEN_GROUP = ":open_group";
            static const std::string CLOSE_GROUP = ":close_group";

            static const std::string LONG_TXN = LONG + TXN;
            static const std::string SHORT_TXN = SHORT + TXN;
        }
    }
} /* iqt */
#endif /* end of include guard: _IQT_CONST_HPP_ */
