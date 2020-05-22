#include "stgy/portal/portal_stgy.h"

#include "stgy/stgy_context.h"
#include "stgy/abs_stgy.h"
#include "stgy/stgy_executor.h"
#include "stgy/single_grid/single_grid_stgy.h"
#include "stgy/single_grid/single_grid_stgy_helper.hpp"
#include "stgy/time_line/time_line_stgy.h"
#include "stgy/time_line/time_line_stgy_helper.hpp"

namespace iqt {
    namespace stgy {
        namespace portal {
            using namespace base::portal;

            InitializerCollector g_collector;

            PYBIND11_MODULE(portal_stgy, m) {
                m.doc() = "pybind stgy portal plugin";

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
            }

            Portal::Portal() {

                g_collector.append([](py::module &m) {
//                    py::class_<AbstractStgy>(m, "AbstractStgy");

                    py::class_<AbstractStgy, PyAbstractStgy>(m, "AbstractStgy")
                            .def(py::init<>())
                            .def("set_stgy_context", &AbstractStgy::set_stgy_context, py::keep_alive<1, 2>())
                            .def("init", &AbstractStgy::init)
                            .def("handle_tick", &AbstractStgy::handle_tick)
                            .def("before_trading", &AbstractStgy::before_trading)
                            .def("after_trading", &AbstractStgy::after_trading);

                    py::class_<SingleGridConfig>(m, "SingleGridConfig")
                            .def(py::init<>())
                            .def_readwrite("stgy_type", &SingleGridConfig::stgy_type)
                            .def_readwrite("stgy_id", &SingleGridConfig::stgy_id)
                            .def_readwrite("version", &SingleGridConfig::version)
                            .def_readwrite("universe", &SingleGridConfig::universe)
                            .def_readwrite("log_level", &SingleGridConfig::log_level)
                            .def_readwrite("slots", &SingleGridConfig::slots)
                            .def_readwrite("open_spread", &SingleGridConfig::open_spread)
                            .def_readwrite("close_spread", &SingleGridConfig::close_spread)
                            .def_readwrite("close_today", &SingleGridConfig::close_today);

                    py::class_<TimeLineConfig>(m, "TimeLineConfig")
                            .def(py::init<>())
                            .def_readwrite("stgy_type", &TimeLineConfig::stgy_type)
                            .def_readwrite("stgy_id", &TimeLineConfig::stgy_id)
                            .def_readwrite("version", &TimeLineConfig::version)
                            .def_readwrite("universe", &TimeLineConfig::universe)
                            .def_readwrite("log_level", &TimeLineConfig::log_level)
                            .def_readwrite("slot", &TimeLineConfig::slot)
                            .def_readwrite("spread", &TimeLineConfig::spread)
                            .def_readwrite("close_today", &TimeLineConfig::close_today);

                    py::class_<SingleGridStgy, AbstractStgy>(m, "SingleGridStgy")
                            .def(py::init<SingleGridConfig *>(), py::keep_alive<1, 2>())
                            .def("update_signal", &SingleGridStgy::update_signal);

                    py::class_<TimeLineStgy, AbstractStgy>(m, "TimeLineStgy")
                            .def(py::init<TimeLineConfig *>(), py::keep_alive<1, 2>());

                    py::class_<StgyContext>(m, "StgyContext", py::dynamic_attr())
                            .def(py::init<>())
                            .def_property("universe", &StgyContext::get_universe, &StgyContext::set_universe);

                    py::class_<StgyExecutor>(m, "StgyExecutor")
                            .def(py::init<bool>())
                            .def("init", &StgyExecutor::init)
                            .def("start", &StgyExecutor::start)
                            .def_static("start_event_source", &StgyExecutor::start_event_source)
                            .def("append_stgy", &StgyExecutor::append_stgy, py::keep_alive<1, 2>()/*, py::keep_alive<1, 3>()*/);
                });
            }

        } /* portal */
    } /* stgy */
} /* iqt */ 
