#ifndef _IQT_MODEL_TICK_H_
#define _IQT_MODEL_TICK_H_

#include <string>
#include <chrono>

namespace iqt {
    namespace model {
        using std::chrono::system_clock;

        struct Bar {
            std::string order_book_id;
            system_clock::time_point datetime;
            double open;
            double close;
            double low;
            double high;
            int volume;

            double limit_up;
            double limit_down;


//            system_clock::time_point rcvTime = system_clock::now();

        };


    } /* model  */

} /* iqt */
#endif /* end of include guard: _IQT_MODEL_TICK_H_ */
