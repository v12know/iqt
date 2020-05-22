#ifndef _IQT_TRADE_OBSERVER_STGY_OBSERVER_H_
#define _IQT_TRADE_OBSERVER_STGY_OBSERVER_H_

#include <mutex>
#include <condition_variable>
#include <string>
#include <unordered_map>
#include <list>
#include <memory>

namespace iqt {
    namespace model {
        class Tick;
    }

    namespace trade {
        namespace observer {

            class StgyObserver {
            public:
                StgyObserver(bool drop_tick=true);
//                virtual ~StgyObserver() {}

//	void update(const std::shared_ptr<model::Tick> tick);
                std::shared_ptr<std::list<std::shared_ptr<model::Tick>>> wait();

                void notify(const std::shared_ptr<model::Tick> tick);

            private:
                bool drop_tick_;
                std::mutex mutex_;
                std::condition_variable new_tick_cond_;
                std::shared_ptr<std::list<std::shared_ptr<model::Tick>>> tick_list_;
            };

        } /* observer */
    } /* trade */

} /* iqt */
#endif /* end of include guard: _IQT_TRADE_OBSERVER_STGY_OBSERVER_H_ */
