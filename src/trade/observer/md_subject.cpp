#include "trade/observer/md_subject.h"

#include <iostream>
#include "base/log.h"
#include "model/tick.h"

namespace iqt {

    namespace trade {
        namespace observer {

            MdSubject *MdSubject::instance() {
                static MdSubject s_MdSubject;
                return &s_MdSubject;
            }

            void MdSubject::attach(const std::string &order_book_id, StgyObserver *o) {
//	std::cout << "attach order_book_id=" << order_book_id << "observer=" << o << std::endl;
                auto iter = observer_map_.find(order_book_id);
                if (iter != observer_map_.end()) {
                    auto &observerList = iter->second;
                    observerList.push_back(o);
                } else {
                    std::list<StgyObserver *> observerList;
                    observerList.push_back(o);
                    observer_map_[order_book_id] = observerList;
                }
            }

            void MdSubject::dettach(const std::string &order_book_id, StgyObserver *o) {
                auto iter = observer_map_.find(order_book_id);
                if (iter != observer_map_.end()) {
                    auto observerList = iter->second;
                    observerList.remove(o);
                }
            }

            void MdSubject::notify(const std::shared_ptr<model::Tick> tick) {
                auto iter = observer_map_.find(tick->order_book_id);
                //log_trace("notifyhehee111, order_book_id={0}", order_book_id);
                if (iter != observer_map_.end()) {
                    //log_trace("notifyhehee");
                    auto &observer_list = iter->second;
                    for (auto *o : observer_list) {
//			std::cout << "update order_book_id=" << order_book_id << "observer=" << o << std::endl;
                        o->notify(tick);
                    }
                }

            }

        } /* observer */
    } /* trade */

} /* iqt */ 
