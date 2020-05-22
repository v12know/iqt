#include "portal/portal_iqt.h"

#include <pybind11/chrono.h>
#include "event/event.h"

#include "base/log.h"
#include "model/tick.h"
#include "base/redis.h"
#include "base/sig_catch.hpp"
#include "env.h"
#include "model/order.h"

namespace iqt {
    namespace portal {
        using namespace base::portal;

        InitializerCollector g_collector;

        PYBIND11_MODULE(portal_iqt, m) {
            m.doc() = "pybind iqt portal plugin";

            static Portal s_portal;
#if !defined(NDEBUG)
            m.attr("debug_enabled") = true;
            g_collector.bind_ConstructorStats(m);
#else
            m.attr("debug_enabled") = false;
#endif

            for (const auto &initializer : g_collector.get_collector())
                initializer(m);

            if (!py::hasattr(m, "have_eigen"))
                m.attr("have_eigen") = false;
//            std::cout << global::make_order_book_id("ZC001")
//                      << global::make_order_book_id("ZC805")
//                    << global::make_order_book_id("ZC905")
//                    << global::make_order_book_id("rb1905")
//                    << global::make_order_book_id("rb2005")
//                      << global::make_order_book_id("rb1805") << std::endl;

        }


        Portal::Portal() {

            g_collector.append([](py::module &m) {
                py::class_<Config>(m, "Config", py::dynamic_attr())
                        .def(py::init<>())
                        .def_property("run_type", &Config::get_run_type, &Config::set_run_type)
                        .def_readwrite("start_date", &Config::start_date)
                        .def_readwrite("future_starting_cash", &Config::future_starting_cash)
                        .def_readwrite("margin_multiplier", &Config::margin_multiplier);
                py::class_<Env>(m, "Env", py::dynamic_attr())
//                        .def("instance", &Env::instance, py::return_value_policy::reference)
                        .def_property_static("config",
                                             [](py::object) { return Env::instance()->config; },
                                             [](py::object, const Config &config) {
                                                 Env::instance()->config = config;
                                             })
                        .def_property_static("universe",
                                             [](py::object) { return Env::instance()->universe; },
                                             [](py::object, const std::unordered_set<std::string> &universe) {
                                                 Env::instance()->universe = universe;
                                             })
                        .def_property_readonly_static("instruments", [](py::object) {
                            return *Env::instance()->instruments;
                        })
                        .def_property_readonly_static("trading_date", [](py::object) {
                            return Env::instance()->trading_date;
                        });
            });
            g_collector.append([](py::module &m) {
                py::class_<event::EventBus>(m, "EventBus")
//                        .def("instance", &event::EventBus::instance, py::return_value_policy::reference)
                        .def_static("add_listener_order", [](const std::string &event_type, py::function listener,
                                                             const std::string &target_id) {
                            event::EventBus::add_listener<model::OrderPtr>(event_type, {
                                    [listener](model::OrderPtr order) {
                                        listener(order);
                                    }, target_id
                            });
                        })
                        .def_static("publish_event_universe",
                                    [](const std::string event_type, std::vector<std::string> &universe) {

                                        auto event = std::make_shared<event::Event<std::vector<std::string> &>>(
                                                event_type, universe);
                                        std::cout << "publish_event_universe" << std::endl;
                                        event::EventBus::publish_event(event);
                                    });
/*                        .def_static("publish_event_universe", [](std::string event_name, std::shared_ptr<std::vector<std::string>> universe) {
                            auto event_bus = event::EventBus::instance();
                            auto event = std::make_shared<event::Event<std::vector<std::string>>>(
                                    event_name, universe);
                            event_bus->publish_event(event);
                        });*/
            });

            g_collector.append([](py::module &m) {
                using namespace base;
                py::class_<LogFactory>(m, "LogFactory")
//                    .def(py::init<>())
//                        .def("instance", &LogFactory::instance, py::return_value_policy::reference)
                        .def("init", &LogFactory::init);

                using namespace base::redis;
                py::class_<RedisFactory>(m, "RedisFactory")
//                        .def_static("instance", &RedisFactory::instance, py::return_value_policy::reference)
                        .def_static("create_redis", &RedisFactory::create_redis, "host"_a, "port"_a, "db"_a,
                                    "password"_a);
            });

            g_collector.append([](py::module &m) {
                using iqt::model::Instrument;
                py::class_<Instrument, std::shared_ptr<Instrument>>(m, "Instrument")
                        .def(py::init<>())
                        .def_readwrite("order_book_id", &Instrument::order_book_id)
                        .def_readwrite("instrument_id", &Instrument::instrument_id)
                        .def_readwrite("underlying_symbol", &Instrument::underlying_symbol)
                        .def_readwrite("exchange_id", &Instrument::exchange_id)
                        .def_readwrite("expire_date", &Instrument::expire_date)
                        .def_readwrite("long_margin_ratio", &Instrument::long_margin_ratio)
                        .def_readwrite("short_margin_ratio", &Instrument::short_margin_ratio)
                        .def_readwrite("contract_multiplier", &Instrument::contract_multiplier)
                        .def_readwrite("price_tick", &Instrument::price_tick)
                        .def_readwrite("margin_type", &Instrument::margin_type);

                using iqt::model::Order;
                py::class_<Order, std::shared_ptr<Order>>(m, "Order")
                        .def(py::init<>())
                        .def_readwrite("create_time", &Order::create_time)
                        .def_readwrite("trading_date", &Order::trading_date)
                        .def_readwrite("order_book_id", &Order::order_book_id)
                        .def_readwrite("front_id", &Order::front_id)
                        .def_readwrite("session_id", &Order::session_id)
                        .def_readwrite("order_id", &Order::order_id)
                        .def_readwrite("order_sys_id", &Order::order_sys_id)
                        .def_readwrite("exchange_id", &Order::exchange_id)
                        .def_readwrite("side", &Order::side)
                        .def_readwrite("position_effect", &Order::position_effect)
                        .def_readwrite("status", &Order::status)
                        .def_readwrite("avg_price", &Order::avg_price)
                        .def_readwrite("filled_quantity", &Order::filled_quantity)
                        .def_readwrite("trade_filled_quantity", &Order::trade_filled_quantity)
                        .def_readwrite("price", &Order::price)
                        .def_readwrite("quantity", &Order::quantity)
                        .def_readwrite("type", &Order::type)
                        .def_readwrite("min_quantity", &Order::min_quantity)
                        .def_readwrite("transaction_cost", &Order::transaction_cost)
                        .def_readwrite("order_key", &Order::order_key)
                        .def_readwrite("group_key", &Order::group_key)
                        .def_readwrite("rpt_key", &Order::rpt_key)
                        .def_readwrite("order_sys_key", &Order::order_sys_key)
                        .def_readwrite("message", &Order::message);

                using iqt::model::Tick;
                using std::chrono::system_clock;
                auto tick = py::class_<Tick, std::shared_ptr<Tick> >(m, "Tick"/*, py::dynamic_attr()*/)
                        .def(py::init<>())
                        .def(py::init<const Tick &>())
                        .def_readwrite("order_book_id", &Tick::order_book_id)
                        .def_readwrite("datetime", &Tick::datetime)
                        .def_readwrite("last", &Tick::last)
                        .def_readwrite("volume", &Tick::volume)
                        .def_readwrite("total_turnover", &Tick::total_turnover)
                        .def_readwrite("open_interest", &Tick::open_interest)
                        .def_readwrite("b1", &Tick::b1)
                        .def_readwrite("b1_v", &Tick::b1_v)
                        .def_readwrite("a1", &Tick::a1)
                        .def_readwrite("a1_v", &Tick::a1_v)
                        .def_readwrite("trading_date", &Tick::trading_date)
                        .def_readwrite("open", &Tick::open)
                        .def_readwrite("low", &Tick::low)
                        .def_readwrite("high", &Tick::high)
                        .def_readwrite("prev_close", &Tick::prev_close)
                        .def_readwrite("prev_settlement", &Tick::prev_settlement)
                        .def_readwrite("limit_up", &Tick::limit_up)
                        .def_readwrite("limit_down", &Tick::limit_down)
                        .def_readwrite("settlement", &Tick::settlement);
//                        .def_readwrite("iqt_symbol", &Tick::iqt_symbol);
                tick.def("__getstate__", [](const Tick &md) {
                            return py::make_tuple(md.order_book_id, md.datetime, md.last, md.volume,
                                                  md.total_turnover,
                                                  md.open_interest, md.b1, md.b1_v, md.a1, md.a1_v,
                                                  md.trading_date, md.open, md.low, md.high, md.prev_close,
                                                  md.prev_settlement, md.limit_up, md.limit_down, md.settlement);
                        })
                        .def("__setstate__", [](Tick &md, py::tuple t) {
                            if (t.size() != 19)
                                throw std::runtime_error("Invalid state!");
                            new(&md) Tick();
                            md.order_book_id = t[0].cast<std::string>();
                            md.datetime = t[1].cast<system_clock::time_point>();
                            md.last = t[2].cast<double>();
                            md.volume = t[3].cast<int>();
                            md.total_turnover = t[4].cast<double>();
                            md.open_interest = t[5].cast<double>();
                            md.b1 = t[6].cast<double>();
                            md.b1_v = t[7].cast<int>();
                            md.a1 = t[8].cast<double>();
                            md.a1_v = t[9].cast<int>();
                            md.trading_date = t[10].cast<int>();
                            md.open = t[11].cast<double>();
                            md.low = t[12].cast<double>();
                            md.high = t[13].cast<double>();
                            md.prev_close = t[14].cast<double>();
                            md.prev_settlement = t[15].cast<double>();
                            md.limit_up = t[16].cast<double>();
                            md.limit_down = t[17].cast<double>();
                            md.settlement = t[18].cast<double>();
                        })
                        .def("fix_settlment", &Tick::fix_settlement)
                        .def("fix_tick", &Tick::fix_tick)
                .def("__str__", &Tick::to_string);
                //.def("__repr__", [](const Tick &d) { return std::string("tick.lastPrice"); });

            });


        }
    } /* portal */
} /* iqt */
