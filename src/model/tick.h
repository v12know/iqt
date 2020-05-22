#ifndef _IQT_MODEL_TICK_H_
#define _IQT_MODEL_TICK_H_

#include <string>
#include <sstream>
#include <chrono>
#include <unordered_map>
#include <base/lock/rwlock.hpp>
#include <base/lock/spin_mutex.hpp>

#include "ctp/ThostFtdcUserApiStruct.h"
//#include "env.h"
#include "base/util.hpp"
#include "base/lock/rwlock.hpp"
#include "global.h"
//#include "position.h"

namespace iqt {
    namespace model {
        using std::chrono::system_clock;

        struct Tick {
            std::string order_book_id;
            system_clock::time_point datetime;
            double open = 0;
            double last;
            double low;
            double high;
            double prev_close;
            int volume = 0;
            double total_turnover = 0;
            double open_interest = 0;
            double settlement = 0;
            double prev_settlement;

            double b1;
            double b2 = 0;
            double b3 = 0;
            double b4 = 0;
            double b5 = 0;

            int b1_v;
            int b2_v = 0;
            int b3_v = 0;
            int b4_v = 0;
            int b5_v = 0;

            double a1;
            double a2 = 0;
            double a3 = 0;
            double a4 = 0;
            double a5 = 0;

            int a1_v;
            int a2_v = 0;
            int a3_v = 0;
            int a4_v = 0;
            int a5_v = 0;

            double limit_up;
            double limit_down;

            std::string trading_date;
//            std::string iqt_symbol;//一般source+order_book_id(如f.rb1805)

//            system_clock::time_point rcv_time = system_clock::now();

            Tick() = default;

            Tick(const Tick &md) = default;

            Tick(Tick &&md) noexcept = default;

            Tick(const CThostFtdcDepthMarketDataField *pData, const std::string &_action_date, const std::string &_trading_date) :
                    order_book_id(global::make_order_book_id(pData->InstrumentID)),
                    datetime(formatDatetime(_action_date.c_str(), pData->UpdateTime, pData->UpdateMillisec)),
                    open(pData->OpenPrice), last(pData->LastPrice),
                    low(pData->LowestPrice), high(pData->HighestPrice),
                    prev_close(pData->PreClosePrice), volume(pData->Volume), total_turnover(pData->Turnover),
                    open_interest(pData->OpenInterest), prev_settlement(pData->PreSettlementPrice),
                    b1(pData->BidPrice1), b1_v(pData->BidVolume1), a1(pData->AskPrice1), a1_v(pData->AskVolume1),
                    limit_up(pData->UpperLimitPrice), limit_down(pData->LowerLimitPrice),
                    trading_date(_trading_date)/*, iqt_symbol("f." + order_book_id)*/ {}

            Tick(double _last) : last(_last), limit_up(_last * 1.1), limit_down(_last * .9) {}

            static system_clock::time_point
            formatDatetime(const char *pTradingDay, const char *pUpdateTime, const int &updateMillisec) {
                char pfDatetime[20];
                sprintf(pfDatetime, "%s%s", pTradingDay, pUpdateTime);

                auto new_time = base::strptimep(pfDatetime, "%Y%m%d%H:%M:%S");
                return new_time + std::chrono::milliseconds(updateMillisec);
            }


            std::string to_string() const {
                std::ostringstream oss;
                oss << "order_book_id=" << order_book_id << ", datetime=" << base::strftimep(datetime) << "." << base::ms(datetime)
                    << ", b1=" << b1 << ", b1_v=" << b1_v << ", open=" << open << ", last=" << last << ", low=" << low << ", high=" << high
                    << ", pre_close=" << prev_close << ", volume=" << volume << ", total_turnover=" << total_turnover
                    << ", open_interest=" << open_interest << ", settlement=" << settlement << ", prev_settlement=" << prev_settlement << ", a1=" << a1
                    << ", a1_v=" << a1_v << ", limit_up=" << limit_up << ", limit_down=" << limit_down << ", trading_date=" << trading_date;
                return oss.str();
            }
            //	static std::string formatDatetime(const char *pTradingDay, const char *pUpdateTime, const int &updateMillisec) {
            //		char pfDatetime[20];
            //		sprintf(pfDatetime, "%s%s%03d",pTradingDay, pUpdateTime, updateMillisec);
            //		return pfDatetime;
            //	}


            /***
             * fix tick's settlment field
             */
            void fix_settlement();

            /***
             * fix tick's settlment field and czce's datetime field
             */
            void fix_tick();
        };


        class Snapshot {
        private:
            std::unordered_map<std::string, std::shared_ptr<Tick>> tick_map_;
            base::lock::RWLock lock_;
        public:
            std::shared_ptr<Tick> &operator[](const std::string &order_book_id) {
                lock_.read_guard();
                return tick_map_[order_book_id];
            }
            std::shared_ptr<Tick> &operator[](std::string &&order_book_id) {
                lock_.read_guard();
                return tick_map_[std::move(order_book_id)];
            }
            void set_tick(std::shared_ptr<Tick> tick) {
                lock_.write_guard();
                tick_map_[tick->order_book_id] = std::move(tick);
            }
            void reserve(size_t __n)
            { tick_map_.reserve(__n); }
            auto count(const std::string& order_book_id) const -> decltype(tick_map_.count(order_book_id))
            { return tick_map_.count(order_book_id); }
            auto begin() noexcept -> decltype(tick_map_.begin())
            { return tick_map_.begin(); }
            auto begin() const noexcept -> decltype(tick_map_.begin())
            { return tick_map_.begin(); }
            auto cbegin() const noexcept -> decltype(tick_map_.cbegin())
            { return tick_map_.cbegin(); }
            auto end() noexcept -> decltype(tick_map_.end())
            { return tick_map_.end(); }
            auto end() const noexcept -> decltype(tick_map_.end())
            { return tick_map_.begin(); }
            auto cend() const noexcept -> decltype(tick_map_.cend())
            { return tick_map_.cend(); }

        };
    } /* model  */

} /* iqt */
#endif /* end of include guard: _IQT_MODEL_TICK_H_ */
