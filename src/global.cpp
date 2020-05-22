#include "global.h"

#include "base/util.hpp"
//#include "base/log.h"

#include "env.h"

namespace iqt {

namespace global {

//std::atomic_int gRequestId(0);

        double margin_of(const std::string &order_book_id, int quantity, double price) {
            auto instrument = Env::instance()->get_instrument(order_book_id);
            auto marigin_rate = instrument->long_margin_ratio * Env::instance()->config.margin_multiplier;
            return quantity * instrument->contract_multiplier * price * marigin_rate;
        }

        char get_year3(const std::string &instrument_id) {
            std::time_t t = std::time(NULL);
            std::tm *ptm = std::localtime(&t);
            char last3 = instrument_id[instrument_id.size() - 3];
            char datetimeStr[8];
            std::strftime(datetimeStr, sizeof(datetimeStr), "%Y", ptm);
            char year3 = datetimeStr[2];
            return last3 >= datetimeStr[3] ? year3 : year3 + 1;
        }

} /* global */
} /* iqt */ 
