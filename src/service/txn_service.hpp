#ifndef _IQT_SERVICE_TXN_SERVICE_HPP_
#define _IQT_SERVICE_TXN_SERVICE_HPP_

#include <cassert>

#include "base/util.hpp"
#include "model/txn.h"
#include "model/rpt.h"
#include "dao/txn_dao.hpp"
//#include "base/redis.h"
#include "base/log.h"


namespace iqt {
    namespace service {
//    using namespace base::redis;
        using namespace model;
        using namespace dao;

        class TxnService {
        public:
            TxnService(const std::string &log_name) : log_name_(log_name) {}

            void load_txn(cpp_redis::client &r, Txn &txn) {
                if (txn_dao_.exists(r, {txn.txn_key})) {
                    txn_dao_.get_txn(r, txn);
                    txn_dao_.get_txn_lock(r, txn);
                } else {
                    txn_dao_.add_txn(r, txn);
                    txn_dao_.add_txn_lock(r, txn);
                }
                log_info_g(log_name_, "load_txn finish, txn_key={0}, direct={1}, status={2}", txn.txn_key,
                           txn::s_direct_type.enum2literal(txn.direct), txn::s_status_type.enum2literal(txn.status));
            }

            void update_txn(cpp_redis::client &r, Txn &txn) {
                txn_dao_.update_txn(r, txn);
            }

            void update_txn_lock(cpp_redis::client &r, Txn &txn) {
                txn_dao_.update_txn_lock(r, txn);
            }


/*        void exit_txn(cpp_redis::client &r, TxnPtr &txn, Rpt &rpt) {
            txn->status = txn::STATUS_TYPE::EXIT;
            update_txn(r, txn);
            txn_dao_.sadd(r, rpt.rpt_key + )
        }*/

            std::vector<model::SlotPtr>
            get_txn_slots(cpp_redis::client &r, const std::string &txn_slots_key,
                          const std::vector<int> &config_slots) {
                std::vector<model::SlotPtr> txn_slots;
                if (txn_dao_.exists(r, {txn_slots_key})) {
                    log_info_g(log_name_, "exists txn_slots_key = {0}", txn_slots_key);
                    auto tmp_slots = txn_dao_.get_txn_slots(r, txn_slots_key);

                    bool changed = false;
                    int slot_open_sum = 0;
                    if (txn_slots.size() > config_slots.size()) {
                        changed = true;
                    }
                    for (size_t i = 0; i < tmp_slots.size(); ++i) {
                        if (!changed && tmp_slots[i]->capacity != config_slots[i]) {
                            changed = true;
                        }
                        slot_open_sum += tmp_slots[i]->open;
                        assert(!tmp_slots[i]->opening);
//                    PPK_ASSERT_ERROR(!tmp_slots[i]->opening, "第%d个slot的正在开仓数不为0, txn_slots_key=%s",
//                                     static_cast<int>(i), txn_slots_key.c_str());
                    }
                    int slot_cap_sum = 0;
                    for (auto cap : config_slots) {
                        slot_cap_sum += cap;
                    }
                    assert(slot_open_sum <= slot_cap_sum);
/*                    PPK_ASSERT_ERROR(slot_open_sum <= slot_cap_sum, "配置文件的slots数量小于实际开仓的slots数量,"
                        " txn_slots_key=%s, slot_open_sum=%d, slot_cap_sum=%d", txn_slots_key.c_str(), slot_open_sum, slot_cap_sum);*/
                    if (changed) {
                        log_info_g(log_name_, "配置文件的slots数量和实际开仓的slots数量不匹配，必须重新分配");
                        size_t i = 0;
                        do {
                            if (!extend_txn_slots(txn_slots, config_slots)) break;
                            for (; i < txn_slots.size(); ++i) {
                                auto slot = txn_slots[i];
                                if (slot_open_sum >= slot->capacity) {
                                    slot->open = slot->capacity;
                                } else {
                                    slot->open = slot_open_sum;
                                }
                                slot_open_sum -= slot->capacity;
                                if (slot_open_sum <= 0) {
                                    break;
                                }
                            }
                        } while (slot_open_sum > 0);
                        txn_dao_.reset_txn_slots(r, txn_slots_key, txn_slots);
                    } else {
                        txn_slots = tmp_slots;
                        if (shrink_txn_slots(txn_slots)) {
                            txn_dao_.reset_txn_slots(r, txn_slots_key, txn_slots);
                        }
                    }
                } else {//new slots
                    log_info_g(log_name_, "not exists txn_slots_key = {0}", txn_slots_key);
                    extend_txn_slots(txn_slots, config_slots);
                    txn_dao_.reset_txn_slots(r, txn_slots_key, txn_slots);
                }
                return txn_slots;
            }
//        void create_txn_slots(cpp_redis::client &r, )

            int shrink_txn_slots(std::vector<model::SlotPtr> &txn_slots) {
                size_t slots_size = txn_slots.size();
                if (slots_size <= 4) return 0;
                size_t i = 4;
                for (; i < slots_size; ++i) {
                    if (i % 5 == 0 && txn_slots[i]->open == 0) {
                        break;
                    }
                }
                txn_slots.erase(txn_slots.begin() + i, txn_slots.end());
                return slots_size - i;
            }

            int extend_txn_slots(std::vector<model::SlotPtr> &txn_slots, const std::vector<int> &config_slots) {
                int shift = txn_slots.size();
                int sum = 0;
                for (size_t i = 0; i < 5; ++i) {
                    if (shift + i < config_slots.size()) {
                        txn_slots.push_back(std::make_shared<model::Slot>(config_slots[shift + i]));
                        ++sum;
                    } else {
                        log_warn_g(log_name_, "配置文件的slots已经完全分配给实际开仓的slots！！！");
                        break;
                    }
                }
                return sum;
            }

            void append_txn_slots(cpp_redis::client &r, const std::string &txn_slots_key,
                                  int start, const std::vector<SlotPtr> &txn_slots) {
                txn_dao_.append_txn_slots(r, txn_slots_key, start, txn_slots);
            }

            void set_txn_slot(cpp_redis::client &r, const std::string &txn_slots_key, int index,
                              Slot &slot) {
                txn_dao_.set_txn_slot(r, txn_slots_key, index, slot);
            }

            std::list<model::TxnCostPtr>
            get_long_txn_costs(cpp_redis::client &r, const std::string &long_txn_costs_key) {
                if (txn_dao_.exists(r, {long_txn_costs_key})) {
                    return txn_dao_.get_long_txn_costs(r, long_txn_costs_key);
                } else {
                    return {};
                }
            }

            std::list<model::TxnCostPtr>
            get_short_txn_costs(cpp_redis::client &r, const std::string &short_txn_costs_key) {
                if (txn_dao_.exists(r, {short_txn_costs_key})) {
                    return txn_dao_.get_short_txn_costs(r, short_txn_costs_key);
                } else {
                    return {};
                }
            }

            void update_txn_costs(cpp_redis::client &r, const std::string &txn_costs_key,
                                  std::list<TxnCostPtr> &txn_costs) {
                txn_dao_.update_txn_costs(r, txn_costs_key, txn_costs);
            }

            void remove_txn_cost(cpp_redis::client &r, const std::string &txn_costs_key,
                                 const std::string &group_key) {
                txn_dao_.remove_txn_cost(r, txn_costs_key, group_key);
            }

            void
            add_txn_cost(cpp_redis::client &r, const std::string &txn_costs_key, TxnCostPtr txn_cost) {
                txn_dao_.add_txn_cost(r, txn_costs_key, txn_cost);
            }

            void set_handle_group_key(cpp_redis::client &r, const std::string &txn_key,
                                      const std::string &group_key_field, const std::string &group_key,
                                      const std::string &group_zset_key) {
                txn_dao_.hset(r, txn_key, group_key_field, group_key);
                txn_dao_.zadd(r, group_zset_key, {}, {
                        {std::to_string(base::timestamp()), group_key}
                });
            }



/*            void check_handling_group(cpp_redis::client &r, const std::string &txn_key) {
                auto txn_key_open_group_ing = txn_key + suffix::OPEN_GROUP_ING;
                auto txn_key_close_group_ing = txn_key + suffix::CLOSE_GROUP_ING;
                assert(!txn_dao_.exists(r, {txn_key_open_group_ing}));
                assert(!txn_dao_.exists(r, {txn_key_close_group_ing}));
*//*            PPK_ASSERT_ERROR(!txn_dao_.exists(r, {txn_key_open_group_ing}),
                    "上次程序异常关闭，存在未处理完的open_group数据，txn_key_open_group_ing=%s", txn_key_open_group_ing.c_str());
            PPK_ASSERT_ERROR(!txn_dao_.exists(r, {txn_key_close_group_ing}),
                             "上次程序异常关闭，存在未处理完的close_group数据，txn_key_close_group_ing=%s", txn_key_close_group_ing.c_str());*//*
            }*/

        private:
            TxnDao txn_dao_;
            std::string log_name_;
        };

    } /* service */
} /* iqt */
#endif /* end of include guard: _IQT_SERVICE_TXN_SERVICE_HPP_ */
