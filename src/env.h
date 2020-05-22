//
// Created by carl on 17-10-22.
//

#ifndef _IQT_ENV_H_
#define _IQT_ENV_H_

#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <list>

#include "model/instrument.h"
#include "model/tick.h"
//#include "trade/portfolio.h"
//#include "trade/abs_broker.h"
//#include "model/future_info.h"

namespace iqt {

    enum class RUN_TYPE {
        BACKTEST,
        SIM,
        REAL,
        COLLECT,
    };

    struct Config {
        RUN_TYPE run_type;
        void set_run_type(const std::string &run_type);
        std::string get_run_type();
        std::string start_date;

        double future_starting_cash;
        double margin_multiplier = 1;
    };

    namespace model {
        struct Order;
//        struct Instrument;
//        struct Tick;

//        typedef std::shared_ptr<model::Instrument> InstrumentPtr;
//        typedef std::unordered_map<std::string, InstrumentPtr> InstrumentMap;
//        typedef std::shared_ptr<model::InstrumentMap> InstrumentMapPtr;
    }
    namespace trade {
        class Portfolio;
        class AbstractBroker;
    }
    namespace risk {
        class AbstractFrontendValidator;
    }

    struct Env {
        static Env *instance();

        Config config;

        std::unordered_map<std::string, std::shared_ptr<model::Tick>> snapshot;
        std::unordered_set<std::string> universe;
        std::string trading_date;

//        model::FutureInfoMapPtr future_info;

        model::InstrumentMapPtr instruments;
        std::shared_ptr<model::Instrument> get_instrument(const std::string &order_book_id) {
            return instruments->at(order_book_id);
        }

        std::shared_ptr<trade::Portfolio> portfolio;

        std::shared_ptr<trade::AbstractBroker> broker;

        std::list<std::shared_ptr<risk::AbstractFrontendValidator>> frontend_validators;
        void add_frontend_validator(std::shared_ptr<risk::AbstractFrontendValidator> validator) {
            frontend_validators.push_back(validator);
        }
        bool can_submit_order(std::shared_ptr<model::Order> order);
        bool can_cancel_order(std::shared_ptr<model::Order> order);
    };
}


#endif //_IQT_ENV_H_
