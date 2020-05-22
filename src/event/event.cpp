#include "event.h"

//#include "base/exception.h"
#include "base/log.h"
#include "model/order.h"
//#include "base/redis.h"

//#include "event_name.h"


namespace iqt {
    namespace event {

        namespace internal {

            template<typename T>
            void
            DetailEventBus<T>::add_listener(const std::string &event_type, ListenerItem<T> listener_item) {
                auto &listener_items = event_map_[event_type];
                listener_items.emplace_back(std::move(listener_item));
            }

            template<typename T>
            void DetailEventBus<T>::publish_event(std::shared_ptr<Event<T>> event) {
                auto &listener_items = event_map_[event->type];
                std::string target_id = extract_target_id(event);

                for (auto &listener_item : listener_items) {
                    if (listener_item.target_id == target_id || listener_item.target_id == ALL_TARGET_ID) {
                        listener_item.listener(event->data);
                    }
                }
            }

            template<typename T>
            std::string DetailEventBus<T>::extract_target_id(std::shared_ptr<Event<T>>) {
                return ALL_TARGET_ID;
            }

            template<>
            std::string DetailEventBus<model::OrderPtr>::extract_target_id(std::shared_ptr<Event<model::OrderPtr>> event) {
                return event->data->rpt_key;
            }

            template<>
            std::string DetailEventBus<model::TradeTuple &>::extract_target_id(std::shared_ptr<Event<model::TradeTuple &>> event) {
                return std::get<1>(event->data)->rpt_key;
            }

            // Explicit template instantiation for available types, so that the generated
            // library contains them and we can keep the method definitions out of the
            // header file.
            template
            class DetailEventBus<model::OrderPtr>;
            template
            class DetailEventBus<model::TradeTuple &>;
            template
            class DetailEventBus<std::vector<std::string> &>;
            template
            class DetailEventBus<void *>;
        }/* internal */

        EventBus *EventBus::instance() {
            static EventBus s_event_bus;
            return &s_event_bus;
        }

/*        void EventBus::add_listener(
                const std::string &event_name,
                std::function<void(std::shared_ptr<std::vector<std::string>>)> listener, std::string target_id) {
            instance()->str_vec_event_bus_.add_listener(event_name, std::move(listener), std::move(target_id));
        }

        void EventBus::add_listener(
                const std::string &event_name,
                std::function<void(std::shared_ptr<model::Order>)> listener, std::string target_id) {
            instance()->order_event_bus_.add_listener(event_name, std::move(listener), std::move(target_id));
        }*/

    } /* event */
} /* iqt */

