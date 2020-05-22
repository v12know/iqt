#ifndef _IQT_MODEL_COMMISSION_H_
#define _IQT_MODEL_COMMISSION_H_

#include <string>
#include <memory>
#include <unordered_map>

#include <cereal/cereal.hpp>
#include <global.h>

#include "ctp/ThostFtdcUserApiStruct.h"
#include "const.hpp"

namespace iqt {

    namespace model {

        namespace commission {
            static const std::string COMMISSION_LIST = "commission:list:";
            static const std::string COMMISSION_DATE = "commission:date:";
        }

//        struct Margin {
//            std::string order_book_id;
//            double long_margin_ratio;
//            double short_margin_ratio;
//            std::string margin_type;
//        };

        struct Commission {
            std::string order_book_id;
            double open_commission_ratio = 0;
            double close_commission_ratio = 0;
            double close_commission_today_ratio = 0;
            trade::COMMISSION_TYPE commission_type;

            Commission() = default;

            Commission(CThostFtdcInstrumentCommissionRateField *data) :
                    order_book_id(global::make_order_book_id(data->InstrumentID)) {
                if (data->OpenRatioByVolume == 0 && data->CloseRatioByVolume == 0 && data->CloseTodayRatioByVolume == 0) {
                    open_commission_ratio = data->OpenRatioByMoney;
                    close_commission_ratio = data->CloseRatioByMoney;
                    close_commission_today_ratio = data->CloseTodayRatioByMoney;
                    commission_type = trade::COMMISSION_TYPE::BY_MONEY;
                } else {
                    open_commission_ratio = data->OpenRatioByVolume;
                    close_commission_ratio = data->CloseRatioByVolume;
                    close_commission_today_ratio = data->CloseTodayRatioByVolume;
                    commission_type = trade::COMMISSION_TYPE::BY_VOLUME;
                }
            }

            std::string to_string() {
                return order_book_id;
            }

            template<class Archive>
            void serialize(Archive & ar) {
                ar(CEREAL_NVP(order_book_id));
                ar(CEREAL_NVP(open_commission_ratio));
                ar(CEREAL_NVP(close_commission_ratio));
                ar(CEREAL_NVP(close_commission_today_ratio));
                ar(CEREAL_NVP(commission_type));
            }

        };

        struct FutureInfoItem {
            std::string order_book_id;
            double long_margin_ratio = 0;
            double short_margin_ratio = 0;
            trade::MARGIN_TYPE margin_type;

            double open_commission_ratio = 0;
            double close_commission_ratio = 0;
            double close_commission_today_ratio = 0;
            trade::COMMISSION_TYPE commission_type;
        };


        struct FutureInfo {
            FutureInfoItem speculation;
        };

        typedef std::unordered_map<std::string, std::shared_ptr<model::Commission>> CommissionMap;
        typedef std::shared_ptr<model::CommissionMap> CommissionMapPtr;

        typedef std::unordered_map<std::string, std::shared_ptr<model::FutureInfo>> FutureInfoMap;
        typedef std::shared_ptr<model::FutureInfoMap> FutureInfoMapPtr;
    } /* model  */

} /* iqt */
#endif /* end of include guard: _IQT_MODEL_COMMISSION_H_ */
