#ifndef _IQT_MODEL_INSTRUMENT_H_
#define _IQT_MODEL_INSTRUMENT_H_

#include <string>
#include <unordered_map>
#include <memory>
#include <ctime>
#include "global.h"

#include "ctp/ThostFtdcUserApiStruct.h"
#include "const.hpp"

namespace iqt {
    namespace model {

        struct Instrument {
            std::string order_book_id;
            std::string instrument_id;
            std::string underlying_symbol;
            std::string exchange_id;
            std::string expire_date;
            double long_margin_ratio;
            double short_margin_ratio;
            int contract_multiplier;
            double price_tick;
            trade::MARGIN_TYPE margin_type;


            Instrument() = default;
            Instrument(CThostFtdcInstrumentField *pInstrument) :
                    order_book_id(global::make_order_book_id(pInstrument->InstrumentID)), instrument_id(pInstrument->InstrumentID),
                    underlying_symbol(pInstrument->ProductID),
                    exchange_id(pInstrument->ExchangeID), expire_date(pInstrument->ExpireDate),
                    long_margin_ratio(pInstrument->LongMarginRatio), short_margin_ratio(pInstrument->ShortMarginRatio),
                    contract_multiplier(pInstrument->VolumeMultiple),
                    price_tick(pInstrument->PriceTick), margin_type(trade::MARGIN_TYPE::BY_MONEY) {
            }
            Instrument(const Instrument &ins) = default;
            Instrument(Instrument &&ins) = default;

        };

        typedef std::shared_ptr<Instrument> InstrumentPtr;
        typedef std::unordered_map<std::string, InstrumentPtr> InstrumentMap;
        typedef std::shared_ptr<InstrumentMap> InstrumentMapPtr;

    } /* model  */

} /* iqt */
#endif /* end of include guard: _IQT_MODEL_INSTRUMENT_H_ */
