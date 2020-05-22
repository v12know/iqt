#ifndef _BASE_PORTAL_PORTAL_INIT_HPP_
#define _BASE_PORTAL_PORTAL_INIT_HPP_

#include <list>
#include <pybind11/pybind11.h>
#include <functional>

namespace base {
    namespace portal {
        namespace py = pybind11;
        using namespace pybind11::literals;

        class InitializerCollector {
            using Initializer = std::function<void(py::module &)>;
        public:
            InitializerCollector() = default;

            void append(Initializer init);

            void append(const char *submodule_name, Initializer init);

            static void bind_ConstructorStats(py::module &m);

            std::list<Initializer> &get_collector() {
                return inits;
            }
        private:
            std::list<Initializer> inits;
        };

    } /* portal */
} /* base */

#include "base/portal/portal_init_impl.hpp"
#endif /* end of include guard: _BASE_PORTAL_PORTAL_INIT_HPP_ */
