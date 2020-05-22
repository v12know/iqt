#pragma once
#ifndef _IQT_PORTAL_PORTAL_IQT_H_
#define _IQT_PORTAL_PORTAL_IQT_H_

#include "base/portal/portal_init.hpp"
namespace iqt {
    namespace portal {

        using namespace base::portal;
        extern InitializerCollector g_collector;


        class Portal {
        public:
            Portal();
        };
    } /* portal */
} /* iqt */
#endif /* end of include guard: _IQT_PORTAL_PORTAL_IQT_H_ */
