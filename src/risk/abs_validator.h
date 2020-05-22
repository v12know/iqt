//
// Created by carl on 17-11-17.
//

#ifndef _IQT_RISK_ABS_VALIDATOR_H_
#define _IQT_RISK_ABS_VALIDATOR_H_

#include <memory>

namespace iqt {
    namespace model {
        struct Order;
    }
    namespace risk {
        class AbstractFrontendValidator {
        public:
            ~AbstractFrontendValidator() = default;
            virtual bool can_submit_order(std::shared_ptr<model::Order> order) = 0;

            virtual bool can_cancel_order(std::shared_ptr<model::Order> order) = 0;
        };
    }
}

#endif //_IQT_RISK_ABS_VALIDATOR_H_
