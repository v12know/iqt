#include "event_source.h"


#include "base/util.hpp"
#include "base/timer.hpp"
//#include "base/exception.h"
#include "base/log.h"

#include "env.h"
#include "base/redis.h"
#include "dao/base_dao.hpp"
//#include "base/redis.h"

#include "event_type.h"
#include "event.h"

using namespace base::redis;
using std::chrono::system_clock;

namespace iqt {
    namespace event {
        TradeEventSource::TradeEventSource() : timer_(base::Timer::default_timer()),
                                               base_dao_(new dao::BaseDao) {};

        TradeEventSource::~TradeEventSource() {
            delete (timer_);
            delete (base_dao_);
        }

        TradeEventSource *TradeEventSource::instance() {
            static TradeEventSource s_tes;
            return &s_tes;
        }

        void TradeEventSource::start() {
            timer_->schedule(0, 60 * 1000, [&](void *) {
                auto r = RedisFactory::get_redis();
                std::string now_datetime = base::strftimep(system_clock::now(), "%Y%m%d%H%M");
                std::string trading_date = Env::instance()->trading_date;
                if (check_before_trading(*r, trading_date)) {
                    EventBus::publish_event(std::make_shared<event::Event<void *>>(event::BEFORE_TRADING));
//                        EventBus::publish_event(std::make_shared<event::Event<void *>>(event::POST_BEFORE_TRADING));
                    before_trading_date_ = trading_date;
                    base_dao_->set(*r, BEFORE_TRADING_DATE_KEY, before_trading_date_);
                    log_warn("++++++++++++update redis, before_trading_date: {}++++++++++", before_trading_date_);
                }
                if (check_after_trading(*r, now_datetime)) {
                    EventBus::publish_event(std::make_shared<event::Event<void *>>(event::AFTER_TRADING));
                    EventBus::publish_event(std::make_shared<event::Event<void *>>(event::POST_AFTER_TRADING));
                    after_trading_date_ = now_datetime.substr(0, 8);
                    base_dao_->set(*r, AFTER_TRADING_DATE_KEY, after_trading_date_);
                    log_warn("++++++++++++update redis, after_trading_date: {}++++++++++", after_trading_date_);
                }
            });
        }

        bool TradeEventSource::check_before_trading(cpp_redis::client &r, const std::string &trading_date) {
            if (before_trading_date_.empty()) {
                if (base_dao_->exists(r, {BEFORE_TRADING_DATE_KEY})) {
                    before_trading_date_ = base_dao_->get(r, BEFORE_TRADING_DATE_KEY);
                } else {
                    before_trading_date_ = MIN_DATE;
                }
            }
            if (before_trading_date_ < trading_date) {
                return true;
            } else {
                return false;
            }
        }

        bool TradeEventSource::check_after_trading(cpp_redis::client &r, const std::string &now_datetime) {
            auto now_date = now_datetime.substr(0, 8);
            auto now_time = now_datetime.substr(8);
            if (after_trading_date_.empty()) {
                if (base_dao_->exists(r, {AFTER_TRADING_DATE_KEY})) {
                    after_trading_date_ = base_dao_->get(r, AFTER_TRADING_DATE_KEY);
                } else {
                    after_trading_date_ = MIN_DATE;
                }
            }
            if (after_trading_date_ < now_date && now_time >= SETTLEMENT_TIME) {
                return true;
            } else {
                return false;
            }
        }

    } /* event */
} /* iqt */

