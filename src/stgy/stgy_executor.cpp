#include "stgy/stgy_executor.h"

#include "base/log.h"
#include "event/event.h"
#include "event/event_source.h"
//#include "base/exception.h"

#include "trade/observer/stgy_observer.h"
#include "trade/observer/md_subject.h"
#include "stgy/abs_stgy.h"


namespace iqt {
    namespace stgy {
        StgyExecutor::StgyExecutor(bool drop_tick) :
                md_subject_(trade::observer::MdSubject::instance()),
                stgy_observer_(new trade::observer::StgyObserver(drop_tick)) {}

        StgyExecutor::~StgyExecutor() {
            if (thread_ && thread_->joinable())
                thread_->join();
            delete thread_;
            delete stgy_;
        }

        void StgyExecutor::append_stgy(AbstractStgy *stgy) {
            log_trace("StgyExecutor::append_stgy");
//            stgy->set_stgy_context(context);
            if (stgy_group_.empty()) {
                stgy_ = stgy;
            }
            stgy_group_.emplace_back(stgy);
        }

        void StgyExecutor::init() {
            log_trace("StgyExecutor::init");
            std::vector<size_t> idx_vec;
            for (size_t i = 0; i < stgy_group_.size(); ++i) {
                auto &stgy = stgy_group_[i];
                bool success = stgy->init(stgy->get_stgy_context());
                if (!success) {
                    idx_vec.push_back(i);
                    continue;
                }
                const auto &universe = stgy->get_stgy_context()->get_universe();
//                Env::instance()->universe.insert(universe.cbegin(), universe.cend());
                for (auto &order_book_id : universe) {
                    md_subject_->attach(order_book_id, stgy_observer_);
                }
                std::vector<std::string> vec(universe.begin(), universe.end());
                auto event = std::make_shared<event::Event<std::vector<std::string> &>>(event::POST_UNIVERSE_CHANGED, vec);
                event::EventBus::publish_event(event);
            }
            for (auto i = idx_vec.size(); i-- > 0; ) {
                stgy_group_.erase(stgy_group_.begin() + idx_vec[i]);
            }

            event::EventBus::add_listener<void *>(event::BEFORE_TRADING, {
                    [&](void *) {
                        before_trading();
                    }
            });
            event::EventBus::add_listener<void *>(event::AFTER_TRADING, {
                    [&](void *) {
                        after_trading();
                    }
            });
/*            event::EventBus::add_listener<void *>(event::POST_AFTER_TRADING, {
                    [&](std::shared_ptr<void *>) {
                        post_after_trading();
                    }
            });*/
//            trade_event_source_->start();
        }

        void StgyExecutor::start_event_source() {
            event::TradeEventSource::instance()->start();
        }

        void StgyExecutor::before_trading() {
            log_trace("StgyExecutor::before_trading");
            for (auto &stgy : stgy_group_) {
                stgy->before_trading(stgy->get_stgy_context());
            }
        }

        void StgyExecutor::after_trading() {
            log_trace("StgyExecutor::after_trading");
            for (auto &stgy : stgy_group_) {
                stgy->after_trading(stgy->get_stgy_context());
            }
        }

        void StgyExecutor::start() {
            log_trace("StgyExecutor::start");
            run_flag_ = true;
            pause_flag_ = false;
            if (stgy_group_.empty()) {
                MY_THROW(base::BaseException, "StgyExecutor不能执行空的策略");
            } else if (stgy_group_.size() == 1) {
                thread_ = new std::thread(std::mem_fn(&StgyExecutor::run), this);
            } else {
                thread_ = new std::thread(std::mem_fn(&StgyExecutor::run_m), this);
            }
        }

        void StgyExecutor::restart() {
            run_flag_ = false;
            pause_flag_ = true;
            if (thread_ && thread_->joinable())
                thread_->join();
            delete thread_;
            start();
        }

        void StgyExecutor::run() {
            while (run_flag_) {
                auto tick_list = stgy_observer_->wait();
                while (!pause_flag_ && valid_time_flag_) {
//                    log_trace("StgyExecutor::run, before handle_tick");
                    for (auto &tick : *tick_list) {
                        stgy_->handle_tick(stgy_->get_stgy_context(), tick);
                    }
//                    log_trace("StgyExecutor::run, after handle_tick");
                    tick_list = stgy_observer_->wait();
                }
            }
        }

        void StgyExecutor::run_m() {
            while (run_flag_) {
                auto tick_list = stgy_observer_->wait();
                while (!pause_flag_ && valid_time_flag_) {
                    for (auto &tick : *tick_list) {
                        for (auto &stgy : stgy_group_) {
                            if (stgy->get_stgy_context()->get_universe().count(tick->order_book_id) > 0) {
//                            log_trace("StgyExecutor::run_m, before handle_tick");
                                stgy->handle_tick(stgy->get_stgy_context(), tick);
//                            log_trace("StgyExecutor::run_m, after handle_tick");
                            }
                        }
                    }
                    tick_list = stgy_observer_->wait();
                }
            }

        }
    } /* stgy  */
} /* iqt */ 
