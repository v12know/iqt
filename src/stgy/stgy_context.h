#ifndef _IQT_STGY_STGY_CONTEXT_H_
#define _IQT_STGY_STGY_CONTEXT_H_

#include <unordered_set>
#include <string>
//#include "model/tick.h"
#include "trade/portfolio.h"
#include "env.h"

namespace iqt {
/*    namespace trade {
        struct Portfolio;
    }*/
    namespace trade {
        class FutureAccount;
    }

    namespace stgy {

        class StgyContext {
        public:

            std::string now();

            std::shared_ptr<trade::Portfolio> portfolio() {
                return Env::instance()->portfolio;
            }

            std::shared_ptr<trade::FutureAccount> future_account();

            void set_universe(const std::unordered_set<std::string> &universe) {
                universe_ = universe;
            }

            std::unordered_set<std::string> &get_universe() {
                return universe_;
            }
        private:
            std::unordered_set<std::string> universe_;
        };

    } /* stgy  */
} /* iqt */
#endif /* end of include guard: _IQT_STGY_STGY_CONTEXT_H_ */
