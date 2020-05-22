#ifndef _IQT_TRADE_OBSERVER_MD_SUBJECT_H_
#define _IQT_TRADE_OBSERVER_MD_SUBJECT_H_

#include <list>
#include <unordered_map>

#include "trade/observer/stgy_observer.h"
#include "base/macro.hpp"


namespace iqt {

    namespace model {
        class Tick;
    }

    namespace trade {
        namespace observer {

            class MdSubject {
            public:
                static MdSubject *instance();

//                virtual ~MdSubject() {}

                void attach(const std::string &order_book_id, StgyObserver *o);

                void dettach(const std::string &order_book_id, StgyObserver *o);

                void notify(const std::shared_ptr<model::Tick> tick);

                DISALLOW_COPY_AND_ASSIGN(MdSubject);

            private:
                MdSubject() = default;

                std::unordered_map<std::string, std::list<StgyObserver *> > observer_map_;
            };

        } /* observer */
    } /* trade */

} /* iqt */
#endif /* end of include guard: _IQT_TRADE_OBSERVER_MD_SUBJECT_H_ */
