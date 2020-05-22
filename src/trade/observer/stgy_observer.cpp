#include "trade/observer/stgy_observer.h"

#include "base/log.h"
#include "model/tick.h"

namespace iqt {

    namespace trade {
        namespace observer {
            StgyObserver::StgyObserver(bool drop_tick): drop_tick_(drop_tick) {
                tick_list_ = std::make_shared<std::list<std::shared_ptr<model::Tick>>>();
            }

//void StgyObserver::update() {
//	notify();
//}
            std::shared_ptr<std::list<std::shared_ptr<model::Tick>>> StgyObserver::wait() {
                std::unique_lock<std::mutex> lock(mutex_);
                while (tick_list_->size() == 0) {
                    //log_trace("wait for new market data, newCounter_={0}", newCounter_);
                    new_tick_cond_.wait(lock);
                }
                auto new_tick_list = tick_list_;
                tick_list_ = std::make_shared<std::list<std::shared_ptr<model::Tick>>>();
                return new_tick_list;
//                log_trace("wait for new market data");
                //lock.unlock();
            }

            void StgyObserver::notify(const std::shared_ptr<model::Tick> tick) {
                std::unique_lock<std::mutex> lock(mutex_);

//                log_trace("notify new market data come");
                if (drop_tick_) {
                    auto it = std::find_if(tick_list_->begin(), tick_list_->end(),
                                           [&tick](const std::shared_ptr<model::Tick> &el) {
                                               return tick->order_book_id == el->order_book_id;
                                           });
                    if (it != tick_list_->end()) {
/*                    log_trace("find old tick and replace it");
                    log_trace("it: {0}", (*it)->to_string());
                    log_trace("new tick: {0}", tick->to_string());*/
                        *it = tick;
//                    log_trace("after relpace it: {0}", (*it)->to_string());
                    } else {
                        tick_list_->insert(it, tick);
                    }
                } else {
                    tick_list_->emplace_back(tick);
                }

                new_tick_cond_.notify_all();
                //lock.unlock();
            }

        } /* observer */
    } /* trade */

} /* iqt */ 
