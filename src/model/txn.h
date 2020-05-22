#ifndef _IQT_MODEL_TXN_H_
#define _IQT_MODEL_TXN_H_

#include <string>
#include <vector>
#include <memory>

#include "group.h"
#include "base/enum_wrap.hpp"

namespace iqt {
    namespace model {
        namespace txn {
            static const std::string TXN = "txn:";
            static const std::string STGY_ID = "stgy_id";
            static const std::string DIRECT = "direct";
            static const std::string STATUS = "status";
            static const std::string OPEN_COUNT = "open_count";
            static const std::string OPENING_COUNT = "opening_count";
            static const std::string CLOSING_COUNT = "closing_count";

            static const std::string BASE_PRICE = "base_price";
            static const std::string THRESHOLD_PRICE = "threshold_price";
            static const std::string LOCK_DATE = "lock_date";
            static const std::string LOCK_COUNT = "lock_count";
            static const std::string LOCK_PRICE = "lock_price";
            static const std::string UNLOCK_PRICE = "unlock_price";
            static const std::string LOCKABLE_COUNT = "lockable_count";

            static const std::string REALIZED_PNL = "realized_pnl";
            static const std::string HOLDING_PNL = "holding_pnl";
            static const std::string TRANSACTION_COST = "transaction_cost";
            static const std::string LOSS_COUNT = "loss_count";

            static const std::string OPEN_GROUP_KEY = "open_group_key";
            static const std::string CLOSE_GROUP_KEY = "close_group_key";

            enum class DIRECT_TYPE {
                LONG,
                SHORT,
            };

            static base::EnumWrap<DIRECT_TYPE, std::string> s_direct_type({"LONG", "SHORT"});

            enum class STATUS_TYPE {
                NEW,
                READY,
                RUNNING,
                BLOCK,
                LOCKING,
                LOCK,
//                SEMI_LOCK,
                UNLOCKING,
                ONLY_CLOSE,
                STOP,
                EXIT
            };

            static base::EnumWrap<STATUS_TYPE, std::string> s_status_type({"NEW", "READY", "RUNNING", "BLOCK", "LOCKING", "LOCK", "UNLOCKING", "ONLY_CLOSE", "STOP", "EXIT"});
        }

        struct Slot {
            Slot() = delete;
            Slot(int _capacity) : open(0), opening(0), capacity(_capacity) {}
            Slot(int _open, int _opening, int _capacity) :
                    open(_open), opening(_opening), capacity(_capacity) {}
            int open;
            int opening;
//            int closing;
            int capacity;
//            double avg_price = 0;
//            std::vector<std::string> open_groups;
/*            template<class Archive>
            void serialize(Archive & ar) {
                ar(CEREAL_NVP(open));
                ar(CEREAL_NVP(opening));
                ar(CEREAL_NVP(closing));
                ar(CEREAL_NVP(capacity));
            }*/
        };

        typedef std::shared_ptr<model::Slot> SlotPtr;

        struct TxnCost {
//            TxnCost() = delete;
            TxnCost(OpenGroupPtr _open_group, double _avg_cost) : open_group(std::move(_open_group)), avg_cost(_avg_cost) {}
            OpenGroupPtr open_group;
            double avg_cost;
        };

        typedef std::shared_ptr<model::TxnCost> TxnCostPtr;

        struct Txn {
            Txn() = delete;
            Txn(std::string _txn_key, std::string _stgy_id, const txn::DIRECT_TYPE &_direct) :
                    txn_key(std::move(_txn_key)), stgy_id(_stgy_id),
                    direct(_direct), txn_slots_key(txn_key + suffix::SLOT),
                    txn_costs_key(txn_key + suffix::COST), txn_locks_key(txn_key + suffix::LOCK) {}
            std::string txn_key;
            std::string stgy_id;
            txn::DIRECT_TYPE direct;//long or short
            txn::STATUS_TYPE status = txn::STATUS_TYPE::NEW;//'init'未运行 'run'运行中 'end'已经结束
            std::string txn_slots_key;//no persist
            std::string txn_costs_key;//no persist
            std::string txn_locks_key;//no persist
            int open_count = 0;
            int opening_count = 0;
            int closing_count = 0;

            double base_price = 0;
            double threshold_price = 0;
            double old_ma5_ols = 0;//no persist
            double old_ma20_ols = 0;//no persist
            std::string lock_date;//加锁日期 format: yyyymmdd
            int lock_count = 0;
            double lock_price = 0;
            double unlock_price = 0;
            int lockable_count = 0;

            double realized_pnl = 0;
            double holding_pnl = 0;
            double transaction_cost = 0;
            int loss_count = 0;

            std::vector<SlotPtr> txn_slots;
            std::list<TxnCostPtr> txn_costs;
            std::list<TxnCostPtr> txn_locks;

            std::string to_string() const {
                std::ostringstream oss;
                oss << "txn_key=" << txn_key << ", direct=" << txn::s_direct_type.enum2literal(direct) << ", status="
                    << txn::s_status_type.enum2literal(status) << ", base_price=" << base_price << ", txn_slots_key="
                    << txn_slots_key << ", txn_costs_key=" << txn_costs_key << ", open_count=" << open_count
                    << ", lock_count=" << lock_count << ", threshold_pric=" << threshold_price
                    << ", realized_pnl=" << realized_pnl << ", holding_pnl=" << holding_pnl
                    << ", transaction_cost=" << transaction_cost << ", loss_count=" << loss_count;
                return oss.str();
            }
        };
        typedef std::shared_ptr<model::Txn> TxnPtr;

    } /* model  */

} /* iqt */
#endif /* end of include guard: _IQT_MODEL_TXN_H_ */
