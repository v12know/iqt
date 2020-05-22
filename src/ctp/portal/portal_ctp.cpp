#include "ctp/portal/portal_ctp.h"

#include "ctp/md_gateway.h"
#include "ctp/trade_gateway.h"

namespace iqt {
    namespace trade {
        namespace portal {

            using namespace pybind11::literals;

            using namespace base::portal;

            InitializerCollector g_collector;

            PYBIND11_MODULE(portal_ctp, m) {
                m.doc() = "pybind ctp portal plugin";

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

                    py::class_<CtpMdGateway>(m, "CtpMdGateway")
                            .def(py::init<int, int>())
                            .def("connect", &CtpMdGateway::connect, py::call_guard<py::gil_scoped_release>())
                            .def("join", &CtpMdGateway::join, py::call_guard<py::gil_scoped_release>());

                    py::class_<CtpTradeGateway>(m, "CtpTradeGateway")
                            .def(py::init<int, int>())
                            .def("connect", &CtpTradeGateway::connect, py::call_guard<py::gil_scoped_release>())
//                            .def_property("collect_pattern", &CtpTradeGateway::get_collect_pattern,
//                                          &CtpTradeGateway::set_collect_pattern)
                            .def("join", &CtpTradeGateway::join, py::call_guard<py::gil_scoped_release>());

                });
            }
        } /* portal */
    } /* ctp */
} /* iqt */
