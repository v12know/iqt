#ifndef _BASE_PORTAL_PORTAL_INIT_IMPL_HPP_
#define _BASE_PORTAL_PORTAL_INIT_IMPL_HPP_

#include "portal_init.hpp"

#include <list>
#include <pybind11/chrono.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

#include "constructor_stats.hpp"

namespace base {
    namespace portal {

        inline void InitializerCollector::append(Initializer init) {
            inits.emplace_back(std::move(init));
        }

        inline void InitializerCollector::append(const char *submodule_name, Initializer init) {
            inits.emplace_back([=](py::module &parent) {
                auto m = parent.def_submodule(submodule_name);
                init(m);
            });
        }

        inline void InitializerCollector::bind_ConstructorStats(py::module &m) {
            py::class_<ConstructorStats>(m, "ConstructorStats", py::module_local())
                    .def("alive", &ConstructorStats::alive)
                    .def("values", &ConstructorStats::values)
                    .def_readwrite("default_constructions", &ConstructorStats::default_constructions)
                    .def_readwrite("copy_assignments", &ConstructorStats::copy_assignments)
                    .def_readwrite("move_assignments", &ConstructorStats::move_assignments)
                    .def_readwrite("copy_constructions", &ConstructorStats::copy_constructions)
                    .def_readwrite("move_constructions", &ConstructorStats::move_constructions)
                    .def_static("get", (ConstructorStats &(*)(py::object)) &ConstructorStats::get,
                                py::return_value_policy::reference_internal)

                            // Not exactly ConstructorStats, but related: expose the internal pybind number of registered instances
                            // to allow instance cleanup checks (invokes a GC first)
                    .def_static("detail_reg_inst", []() {
                        ConstructorStats::gc();
                        return py::detail::get_internals().registered_instances.size();
                    });
        }

/*        PYBIND11_MODULE(portal_iqt, m) {
            m.doc() = "pybind iqt portal plugin";

#if !defined(NDEBUG)
            m.attr("debug_enabled") = true;
            bind_ConstructorStats(m);
#else
            m.attr("debug_enabled") = false;
#endif

            for (const auto &initializer : initializers())
                initializer(m);

            if (!py::hasattr(m, "have_eigen"))
                m.attr("have_eigen") = false;
        }*/

    } /* portal */
} /* base */
#endif /* end of include guard: _BASE_PORTAL_PORTAL_INIT_IMPL_HPP_ */
