#ifndef _IQT_DAO_TXN_DAO_HPP_
#define _IQT_DAO_TXN_DAO_HPP_

#include <sstream>

#include "dao/base_dao.hpp"
#include "model/txn.h"

namespace iqt {
    namespace dao {
//    using namespace base::redis;
        using namespace model;

        class TxnDao : public BaseDao {
        public:
            void get_txn(cpp_redis::client &r, Txn &txn) {
                auto hmget = r.hmget(txn.txn_key, txn_fields_);
                r.sync_commit();
                auto array = hmget.get().as_array();

                txn.direct = txn::s_direct_type.literal2enum(array[0].as_string());
                txn.status = txn::s_status_type.literal2enum(array[1].as_string());
                txn.open_count = std::stoi(array[2].as_string());
                txn.opening_count = std::stoi(array[3].as_string());
                txn.closing_count = std::stoi(array[4].as_string());

                if (!array[5].is_null())//TODO delete
                    txn.realized_pnl = std::stod(array[5].as_string());
                if (!array[6].is_null())//TODO delete
                    txn.transaction_cost = std::stod(array[6].as_string());
                if (!array[7].is_null())//TODO delete
                    txn.loss_count = std::stoi(array[7].as_string());
            }

            void get_txn_lock(cpp_redis::client &r, Txn &txn) {
                auto hmget = r.hmget(txn.txn_key, txn_lock_fields_);
                r.sync_commit();
                auto array = hmget.get().as_array();

                txn.base_price = std::stod(array[0].as_string());
                txn.threshold_price = std::stod(array[1].as_string());
                txn.lock_date = array[2].as_string();
                txn.lock_count = std::stoi(array[3].as_string());
                txn.lock_price = std::stod(array[4].as_string());
                txn.unlock_price = std::stod(array[5].as_string());
                if (!array[6].is_null())//TODO delete
                    txn.lockable_count = std::stoi(array[6].as_string());
            }

            void update_txn(cpp_redis::client &r, Txn &txn) {
                r.hmset(txn.txn_key, {
                        {txn::OPENING_COUNT, std::to_string(txn.opening_count)},
                        {txn::CLOSING_COUNT, std::to_string(txn.closing_count)},
                        {txn::OPEN_COUNT, std::to_string(txn.open_count)},
                        {txn::STATUS, txn::s_status_type.enum2literal(txn.status)},
                        {txn::REALIZED_PNL, base::to_string(txn.realized_pnl)},
                        {txn::HOLDING_PNL, base::to_string(txn.holding_pnl)},
                        {txn::TRANSACTION_COST, base::to_string(txn.transaction_cost)},
                        {txn::LOSS_COUNT, std::to_string(txn.loss_count)}
                });
            }
            void update_txn_lock(cpp_redis::client &r, Txn &txn) {
                r.hmset(txn.txn_key, {
                        {txn::STATUS, txn::s_status_type.enum2literal(txn.status)},
                        {txn::OPENING_COUNT, std::to_string(txn.opening_count)},
                        {txn::CLOSING_COUNT, std::to_string(txn.closing_count)},
                        {txn::THRESHOLD_PRICE, base::to_string(txn.threshold_price)},
                        {txn::BASE_PRICE, base::to_string(txn.base_price)},
                        {txn::LOCK_DATE, txn.lock_date},
                        {txn::LOCK_COUNT, std::to_string(txn.lock_count)},
                        {txn::LOCK_PRICE, base::to_string(txn.lock_price)},
                        {txn::UNLOCK_PRICE, base::to_string(txn.unlock_price)},
                        {txn::LOCKABLE_COUNT, std::to_string(txn.lockable_count)}
                });
            }

            void add_txn(cpp_redis::client &r, const Txn &txn) {
                r.hmset(txn.txn_key, {
                        {txn::STGY_ID,    txn.stgy_id},
                        {txn::DIRECT,     txn::s_direct_type.enum2literal(txn.direct)},
                        {txn::STATUS,     txn::s_status_type.enum2literal(txn.status)},
                        {txn::OPEN_COUNT, std::to_string(txn.open_count)},
                        {txn::OPENING_COUNT, std::to_string(txn.opening_count)},
                        {txn::CLOSING_COUNT, std::to_string(txn.closing_count)},
                        {txn::REALIZED_PNL, base::to_string(txn.realized_pnl)},
                        {txn::TRANSACTION_COST, base::to_string(txn.transaction_cost)},
                        {txn::LOSS_COUNT, std::to_string(txn.loss_count)}
                });
            }

            void add_txn_lock(cpp_redis::client &r, const Txn &txn) {
                r.hmset(txn.txn_key, {
                        {txn::BASE_PRICE, base::to_string(txn.base_price)},
                        {txn::THRESHOLD_PRICE, base::to_string(txn.threshold_price)},
                        {txn::LOCK_DATE, txn.lock_date},
                        {txn::LOCK_COUNT, std::to_string(txn.lock_count)},
                        {txn::LOCK_PRICE, base::to_string(txn.lock_price)},
                        {txn::UNLOCK_PRICE, base::to_string(txn.unlock_price)},
                        {txn::LOCKABLE_COUNT, std::to_string(txn.lockable_count)}
                });
            }

            static SlotPtr unpack_slot(const std::string &slot_str) {
                auto slot_vec = base::split(slot_str, ",");
                return std::make_shared<Slot>(std::stoi(slot_vec[0]), std::stoi(slot_vec[1]), std::stoi(slot_vec[2]));
            }

            static std::string pack_slot(Slot &slot) {
                std::ostringstream oss;
                oss << slot.open << ',' << slot.opening << ',' << slot.capacity;
                return oss.str();
            }

            void reset_txn_slots(cpp_redis::client &r, const std::string &txn_slots_key,
                                 const std::vector<SlotPtr> &txn_slots) {
                del(r, {txn_slots_key});
                std::vector<std::string> tmp_vec;
                std::transform(txn_slots.begin(), txn_slots.end(), std::back_inserter(tmp_vec),
                               [](const SlotPtr s) {
                                   return pack_slot(*s);
                               });
                r.rpush(txn_slots_key, tmp_vec);
            }

            void append_txn_slots(cpp_redis::client &r, const std::string &txn_slots_key,
                                 int start, const std::vector<SlotPtr> &txn_slots) {
                std::vector<std::string> tmp_vec;
                std::transform(txn_slots.begin() + start, txn_slots.end(), std::back_inserter(tmp_vec),
                               [](const SlotPtr s) {
                                   return pack_slot(*s);
                               });
                r.rpush(txn_slots_key, tmp_vec);
            }

            std::vector<SlotPtr> get_txn_slots(cpp_redis::client &r, const std::string &txn_slots_key) {
                auto lrange = r.lrange(txn_slots_key, 0, -1);
                r.sync_commit();
                auto array = lrange.get().as_array();
                std::vector<SlotPtr> slots;
                for (auto &reply : array) {
                    slots.push_back(unpack_slot(reply.as_string()));
                }
                return slots;
            }

            void set_txn_slot(cpp_redis::client &r, const std::string &txn_slots_key, int index, Slot &slot) {
                r.lset(txn_slots_key, index, pack_slot(slot));
            }

            std::list<TxnCostPtr>
            get_long_txn_costs(cpp_redis::client &r, const std::string &long_txn_costs_key) {
                auto z = r.zrangebyscore(long_txn_costs_key, "-inf", "+inf", true);
                r.sync_commit();
                std::list<TxnCostPtr> tmp_list;
                auto array = z.get().as_array();
                for (size_t i = 0; i < array.size(); i += 2) {
                    auto open_group = std::make_shared<OpenGroup>();
                    open_group->group_key = array[i].as_string();
                    double avg_cost = std::stod(array[i + 1].as_string());
                    tmp_list.push_back(std::make_shared<TxnCost>(std::move(open_group), avg_cost));
                }
                return tmp_list;
            }

            std::list<TxnCostPtr>
            get_short_txn_costs(cpp_redis::client &r, const std::string &short_txn_costs_key) {
                auto z = r.zrevrangebyscore(short_txn_costs_key, "+inf", "-inf", true);
                r.sync_commit();
                std::list<TxnCostPtr> tmp_list;
                auto array = z.get().as_array();
                for (size_t i = 0; i < array.size(); i += 2) {
                    auto open_group = std::make_shared<OpenGroup>();
                    open_group->group_key = array[i].as_string();
                    double avg_cost = std::stod(array[i + 1].as_string());
                    tmp_list.push_back(std::make_shared<TxnCost>(std::move(open_group), avg_cost));
                }
                return tmp_list;
            }
            void update_txn_costs(cpp_redis::client &r, const std::string &txn_costs_key,
                                  std::list<TxnCostPtr> &txn_costs) {
                std::multimap<std::string, std::string> score_members;
                for (auto &txn_cost : txn_costs) {
                    score_members.emplace(base::to_string(txn_cost->avg_cost), txn_cost->open_group->group_key);
                }
                r.zadd(txn_costs_key, {}, score_members);
            }
            void remove_txn_cost(cpp_redis::client &r, const std::string &txn_costs_key, const std::string &group_key) {
                r.zrem(txn_costs_key, {group_key});
            }

            void add_txn_cost(cpp_redis::client &r, const std::string &txn_costs_key, TxnCostPtr txn_cost) {
                r.zadd(txn_costs_key, {}, {
                        { base::to_string(txn_cost->avg_cost), txn_cost->open_group->group_key }
                });
            }

        private:

            std::vector<std::string> txn_fields_ = {txn::DIRECT, txn::STATUS, txn::OPEN_COUNT,
                                                    txn::OPENING_COUNT, txn::CLOSING_COUNT, txn::REALIZED_PNL,
                                                    txn::TRANSACTION_COST, txn::LOSS_COUNT};
            std::vector<std::string> txn_lock_fields_ = {txn::BASE_PRICE, txn::THRESHOLD_PRICE, txn::LOCK_DATE, txn::LOCK_COUNT,
                                                    txn::LOCK_PRICE, txn::UNLOCK_PRICE, txn::LOCKABLE_COUNT};
        };
    } /* dao  */

} /* iqt */

#endif /* end of include guard: _IQT_DAO_TXN_DAO_HPP_ */
