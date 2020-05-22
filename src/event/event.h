#ifndef _IQT_EVENT_BUS_H_
#define _IQT_EVENT_BUS_H_

#include <string>
#include <memory>
#include <list>
#include <unordered_map>
#include <vector>
#include <tuple>

#include "base/macro.hpp"

#include "event_type.h"


namespace iqt {
    namespace model {
        struct Order;
        struct Trade;
        typedef std::shared_ptr<Order> OrderPtr;
        typedef std::shared_ptr<Trade> TradePtr;

        typedef std::tuple<TradePtr, OrderPtr> TradeTuple;
    }
    namespace event {


        template<typename E>
        struct Event {
            std::string type;
            E data;

            Event(std::string _type, E _data = nullptr) :
                    type(std::move(_type)), data(_data) {}
        };

        template<typename E>
        struct ListenerItem {
            typedef std::function<void(E)> listener_t;
            listener_t listener;
            std::string target_id;

            ListenerItem(listener_t _listener, std::string _target_id = ALL_TARGET_ID) :
                    listener(std::move(_listener)), target_id(std::move(_target_id)) {}
        };


        namespace internal {

            template<typename T>
            class DetailEventBus {
            public:
                void add_listener(const std::string &event_type, ListenerItem<T> listener_item);

                void publish_event(std::shared_ptr<Event<T>> event);

                std::string extract_target_id(std::shared_ptr<Event<T>> event);

            private:
                /* data */
                std::unordered_map<std::string, std::list<ListenerItem<T>>> event_map_;
            };
        }/* internal */

        class EventBus {
        public:
            static EventBus *instance();

            template <class T>
            static void add_listener(const std::string &event_type, ListenerItem<T> listener_item) {}

            template <class T>
            static void publish_event(std::shared_ptr<Event<T>> event) {}

            DISALLOW_COPY_AND_ASSIGN(EventBus);

        private:
            EventBus() = default;

            internal::DetailEventBus<std::vector<std::string> &> str_vec_event_bus_;
            internal::DetailEventBus<model::OrderPtr> order_event_bus_;
            internal::DetailEventBus<model::TradeTuple &> trade_event_bus_;
            internal::DetailEventBus<void *> void_event_bus_;
        };


        template <>
        inline void EventBus::add_listener(const std::string &event_type, ListenerItem<std::vector<std::string> &> listener_item) {
            instance()->str_vec_event_bus_.add_listener(event_type, std::move(listener_item));
        }

        template <>
        inline void EventBus::add_listener(const std::string &event_type, ListenerItem<model::OrderPtr> listener_item) {
            instance()->order_event_bus_.add_listener(event_type, std::move(listener_item));
        }

        template <>
        inline void EventBus::add_listener(const std::string &event_type, ListenerItem<model::TradeTuple &> listener_item) {
            instance()->trade_event_bus_.add_listener(event_type, std::move(listener_item));
        }

        template <>
        inline void EventBus::add_listener(const std::string &event_type, ListenerItem<void *> listener_item) {
            instance()->void_event_bus_.add_listener(event_type, std::move(listener_item));
        }

        template <>
        inline void EventBus::publish_event(std::shared_ptr<Event<std::vector<std::string> &>> event) {
            instance()->str_vec_event_bus_.publish_event(event);
        }

        template <>
        inline void EventBus::publish_event(std::shared_ptr<Event<model::OrderPtr>> event) {
            instance()->order_event_bus_.publish_event(event);
        }

        template <>
        inline void EventBus::publish_event(std::shared_ptr<Event<model::TradeTuple &>> event) {
            instance()->trade_event_bus_.publish_event(event);
        }
        template <>
        inline void EventBus::publish_event(std::shared_ptr<Event<void *>> event) {
            instance()->void_event_bus_.publish_event(event);
        }
    } /* event */
} /* iqt */

#endif /* end of include guard: _IQT_EVENT_BUS_H_ */
