//
// Created by carl on 17-10-22.
//

#include "env.h"

#include "base/util.hpp"
#include "risk/abs_validator.h"
#include "base/enum_wrap.hpp"

namespace iqt {

    static base::EnumWrap<RUN_TYPE, std::string> s_run_type({"BACKTEST", "SIM", "REAL", "COLLECT"});

    void Config::set_run_type(const std::string &run_type) {
        this->run_type = s_run_type.literal2enum(run_type);
    }
    std::string Config::get_run_type() {
        return s_run_type.enum2literal(run_type);
    }

    Env *Env::instance() {
        static Env s_env;
        return &s_env;
    }

    bool Env::can_submit_order(std::shared_ptr<model::Order> order) {
        for (auto &v : frontend_validators) {
            if (!v->can_submit_order(order)) {
                return false;
            }
        }
        return true;
    }

    bool Env::can_cancel_order(std::shared_ptr<model::Order> order) {
        for (auto &v : frontend_validators) {
            if (!v->can_cancel_order(order)) {
                return false;
            }
        }
        return true;
    }
}
