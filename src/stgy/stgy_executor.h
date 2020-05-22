#ifndef _IQT_STGY_STGY_EXECUTOR_H_
#define _IQT_STGY_STGY_EXECUTOR_H_

#include <string>
#include <thread>
#include <vector>


namespace iqt {
    namespace trade {
        namespace observer {
            class MdSubject;
            class StgyObserver;
        }
    }
    namespace event {
        class TradeEventSource;
    }

    namespace stgy {
        class AbstractStgy;
//        class StgyContext;

        class StgyExecutor {
        public:
            StgyExecutor(bool drop_tick=true);
            virtual ~StgyExecutor();

            void append_stgy(AbstractStgy *stgy);

            void init();

            void before_trading();

            void after_trading();

            void start();

            void restart();

            static void start_event_source();

        protected:
            void run();
            void run_m();

        private:
            AbstractStgy *stgy_;
            std::vector<AbstractStgy *> stgy_group_;
            std::thread *thread_;

            trade::observer::MdSubject *md_subject_;
            trade::observer::StgyObserver *stgy_observer_;

            bool run_flag_ = true;//线程是否在运行
            bool pause_flag_ = false;//线程是否暂停
            bool valid_time_flag_ = true;//是否在合法时间
        };

    } /* stgy  */
} /* iqt */
#endif /* end of include guard: _IQT_STGY_STGY_EXECUTOR_H_ */
