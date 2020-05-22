#ifndef _IQT_DAO_GROUP_DAO_HPP_
#define _IQT_DAO_GROUP_DAO_HPP_

#include <sstream>

#include "dao/base_dao.hpp"
#include "model/group.h"
#include "model/txn.h"
#include "base/util.hpp"

namespace iqt {
    namespace dao {
//        using namespace base::redis;
//        using namespace model;

        class GroupDao : public BaseDao {
        public:
            GroupDao(int leg_num) : leg_num_(leg_num) {
                open_group_fields_ = {group::UNIT_QUANTITY, group::FILLED_UNIT_QUANTITY, group::AVG_COST, group::TRANSACTION_COST,
                                               group::CLOSE_UNIT_QUANTITY, group::CLOSING_UNIT_QUANTITY, group::SLOT_INDEX, group::GROUP_TYPE};
                for (int i = 0; i < leg_num; ++i) {
                    open_group_fields_.insert(open_group_fields_.end(), {
                            leg::LEG + std::to_string(i) + leg::FILLED_QUANTITY,
                            leg::LEG + std::to_string(i) + leg::AVG_PRICE,
                            leg::LEG + std::to_string(i) + leg::TRANSACTION_COST,
                            leg::LEG + std::to_string(i) + leg::FRAG_STATUS,
                            leg::LEG + std::to_string(i) + leg::ORDER_KEYS
                    });
                }
                close_group_fields_ = {group::UNIT_QUANTITY, group::FILLED_UNIT_QUANTITY, group::AVG_COST, group::TRANSACTION_COST,
                                      group::OPEN_GROUP_KEYS, group::GROUP_TYPE};
                for (int i = 0; i < leg_num; ++i) {
                    open_group_fields_.insert(open_group_fields_.end(), {
                            leg::LEG + std::to_string(i) + leg::FILLED_QUANTITY,
                            leg::LEG + std::to_string(i) + leg::AVG_PRICE,
                            leg::LEG + std::to_string(i) + leg::TRANSACTION_COST,
                            leg::LEG + std::to_string(i) + leg::FRAG_STATUS,
                            leg::LEG + std::to_string(i) + leg::ORDER_KEYS
                    });
                }
            }
            OpenGroupPtr get_open_group(cpp_redis::client &r, const std::string &group_key) {
                auto open_group = std::make_shared<OpenGroup>();
                get_open_group(r, *open_group);
                return open_group;
            }
            CloseGroupPtr get_close_group(cpp_redis::client &r, const std::string &group_key) {
                auto close_group = std::make_shared<CloseGroup>();
                get_close_group(r, *close_group);
                return close_group;
            }

            void get_open_group(cpp_redis::client &r, OpenGroup &open_group) {
                auto hmget = r.hmget(open_group.group_key, open_group_fields_);
                r.sync_commit();
                auto array = hmget.get().as_array();
                open_group.unit_quantity = std::stoi(array[0].as_string());
                open_group.filled_unit_quantity = std::stoi(array[1].as_string());
                open_group.avg_cost = std::stod(array[2].as_string());
                open_group.transaction_cost = std::stod(array[3].as_string());
                open_group.close_unit_quantity = std::stoi(array[4].as_string());
                open_group.closing_unit_quantity = std::stoi(array[5].as_string());
                open_group.slot_index = std::stoi(array[6].as_string());
                open_group.group_type = group::s_group_type_type.literal2enum(array[7].as_string());

                for (int i = 0; i < leg_num_; ++i) {
                    auto leg = std::make_shared<Leg>();
                    leg->filled_quantity = std::stoi(array[8  + i * LEG_COL_NUM].as_string());
                    leg->avg_price = std::stod(array[9 + i * LEG_COL_NUM].as_string());
                    leg->transaction_cost = std::stod(array[10 + i * LEG_COL_NUM].as_string());
                    leg->frag_status = leg::s_frag_status_type.literal2enum(array[11 + i * LEG_COL_NUM].as_string());
                    leg->orders = unpack_orders(array[12 + i * LEG_COL_NUM].as_string());
                    open_group.leg_items.push_back(std::move(leg));
                }
            }
            void get_close_group(cpp_redis::client &r, CloseGroup &close_group) {
                auto hmget = r.hmget(close_group.group_key, close_group_fields_);
                r.sync_commit();
                auto array = hmget.get().as_array();
                close_group.unit_quantity = std::stoi(array[0].as_string());
                close_group.filled_unit_quantity = std::stoi(array[1].as_string());
                close_group.avg_cost = std::stod(array[2].as_string());
                close_group.transaction_cost = std::stod(array[3].as_string());
                close_group.open_groups = unpack_open_groups(array[4].as_string());
                close_group.group_type = group::s_group_type_type.literal2enum(array[5].as_string());

                for (int i = 0; i < leg_num_; ++i) {
                    auto leg = std::make_shared<Leg>();
                    leg->filled_quantity = std::stoi(array[6 + i * LEG_COL_NUM].as_string());
                    leg->avg_price = std::stod(array[7 + i * LEG_COL_NUM].as_string());
                    leg->transaction_cost = std::stod(array[8 + i * LEG_COL_NUM].as_string());
                    leg->frag_status = leg::s_frag_status_type.literal2enum(array[9 + i * LEG_COL_NUM].as_string());
                    leg->orders = unpack_orders(array[10 + i * LEG_COL_NUM].as_string());

                    close_group.leg_items.push_back(std::move(leg));
                }
            }

            static std::vector<OrderPtr> unpack_orders(const std::string &order_keys_str) {
                auto order_key_vec = ::base::split(order_keys_str, ",");
                std::vector<OrderPtr> orders;
                for (auto &order_key : order_key_vec) {
                    auto ord = std::make_shared<Order>();
                    ord->order_key = std::move(order_key);
                    orders.push_back(std::move(ord));
                }
                return orders;
            }

            static std::string pack_orders(const std::vector<OrderPtr> &orders) {
                std::ostringstream oss;
                for (size_t i  = 0; i < orders.size(); ++i) {
                    if (i >= 1) {
                        oss << ',';
                    }
                    oss << orders[i]->order_key;
                }
                return oss.str();
            }

            static std::vector<OpenGroupPtr> unpack_open_groups(const std::string &open_group_keys_str) {
                auto group_key_vec = ::base::split(open_group_keys_str, ",");
                std::vector<OpenGroupPtr> open_groups;
                for (auto &group_key : group_key_vec) {
                    auto open_group = std::make_shared<OpenGroup>();
                    open_group->group_key = std::move(group_key);
                    open_groups.push_back(std::move(open_group));
                }
                return open_groups;
            }
            static std::string pack_open_groups(const std::vector<OpenGroupPtr> &open_groups) {
                std::ostringstream oss;
                for (size_t i  = 0; i < open_groups.size(); ++i) {
                    if (i >= 1) {
                        oss << ',';
                    }
                    oss << open_groups[i]->group_key;
                }
                return oss.str();
            }

            void add_open_group(cpp_redis::client &r, OpenGroup &open_group) {
                std::vector<std::pair<std::string, std::string>> field_val = {
                        {group::TXN_KEY, open_group.txn->txn_key},
                        {group::UNIT_QUANTITY, std::to_string(open_group.unit_quantity)},
                        {group::FILLED_UNIT_QUANTITY, std::to_string(open_group.filled_unit_quantity)},
                        {group::AVG_COST, base::to_string(open_group.avg_cost)},
                        {group::TRANSACTION_COST, base::to_string(open_group.transaction_cost)},
                        {group::CLOSE_UNIT_QUANTITY, std::to_string(open_group.close_unit_quantity)},
                        {group::CLOSING_UNIT_QUANTITY, std::to_string(open_group.closing_unit_quantity)},
                        {group::SLOT_INDEX, std::to_string(open_group.slot_index)},
                        {group::GROUP_TYPE, group::s_group_type_type.enum2literal(open_group.group_type)}
                };
                for (int i = 0; i < leg_num_; ++i) {
                    auto &leg = open_group.leg_items[i];
                    field_val.insert(field_val.end(), {
                            {leg::LEG + std::to_string(i) + leg::FILLED_QUANTITY, std::to_string(leg->filled_quantity)},
                            {leg::LEG + std::to_string(i) + leg::AVG_PRICE, base::to_string(leg->avg_price)},
                            {leg::LEG + std::to_string(i) + leg::TRANSACTION_COST, base::to_string(leg->transaction_cost)},
                            {leg::LEG + std::to_string(i) + leg::FRAG_STATUS, leg::s_frag_status_type.enum2literal(leg->frag_status)},
                            {leg::LEG + std::to_string(i) + leg::ORDER_KEYS, pack_orders(leg->orders)}
                    });
                }
                r.hmset(open_group.group_key, field_val);
            }
            void add_close_group(cpp_redis::client &r, CloseGroup &close_group) {
                std::vector<std::pair<std::string, std::string>> field_val = {
                        {group::TXN_KEY, close_group.txn->txn_key},
                        {group::UNIT_QUANTITY, std::to_string(close_group.unit_quantity)},
                        {group::FILLED_UNIT_QUANTITY, std::to_string(close_group.filled_unit_quantity)},
                        {group::AVG_COST, base::to_string(close_group.avg_cost)},
                        {group::TRANSACTION_COST, base::to_string(close_group.transaction_cost)},
                        {group::OPEN_GROUP_KEYS, pack_open_groups(close_group.open_groups)},
                        {group::GROUP_TYPE, group::s_group_type_type.enum2literal(close_group.group_type)}
                };
                for (int i = 0; i < leg_num_; ++i) {
                    auto &leg = close_group.leg_items[i];
                    field_val.insert(field_val.end(), {
                            {leg::LEG + std::to_string(i) + leg::FILLED_QUANTITY, std::to_string(leg->filled_quantity)},
                            {leg::LEG + std::to_string(i) + leg::AVG_PRICE, ::base::to_string(leg->avg_price)},
                            {leg::LEG + std::to_string(i) + leg::TRANSACTION_COST, base::to_string(leg->transaction_cost)},
                            {leg::LEG + std::to_string(i) + leg::FRAG_STATUS, leg::s_frag_status_type.enum2literal(leg->frag_status)},
                            {leg::LEG + std::to_string(i) + leg::ORDER_KEYS, pack_orders(leg->orders)}
                    });
                }
                r.hmset(close_group.group_key, field_val);
            }

            void update_open_group(cpp_redis::client &r, OpenGroup &open_group) {
                std::vector<std::pair<std::string, std::string>> field_val = {
                        {group::FILLED_UNIT_QUANTITY, std::to_string(open_group.filled_unit_quantity)},
                        {group::CLOSE_UNIT_QUANTITY, std::to_string(open_group.close_unit_quantity)}
                };
                for (int i = 0; i < leg_num_; ++i) {
                    auto &leg = open_group.leg_items[i];
                    field_val.insert(field_val.end(), {
                            {leg::LEG + std::to_string(i) + leg::FILLED_QUANTITY, std::to_string(leg->filled_quantity)},
                            {leg::LEG + std::to_string(i) + leg::FRAG_STATUS, leg::s_frag_status_type.enum2literal(leg->frag_status)},
                            {leg::LEG + std::to_string(i) + leg::ORDER_KEYS, pack_orders(leg->orders)}
                    });
                }
                r.hmset(open_group.group_key, field_val);
            }
            void update_close_group(cpp_redis::client &r, CloseGroup &close_group) {
                std::vector<std::pair<std::string, std::string>> field_val = {
                        {group::FILLED_UNIT_QUANTITY, std::to_string(close_group.filled_unit_quantity)}
                };
                for (int i = 0; i < leg_num_; ++i) {
                    auto &leg = close_group.leg_items[i];
                    field_val.insert(field_val.end(), {
                            {leg::LEG + std::to_string(i) + leg::FILLED_QUANTITY, std::to_string(leg->filled_quantity)},
                            {leg::LEG + std::to_string(i) + leg::FRAG_STATUS, leg::s_frag_status_type.enum2literal(leg->frag_status)},
                            {leg::LEG + std::to_string(i) + leg::ORDER_KEYS, pack_orders(leg->orders)}
                    });
                }
                r.hmset(close_group.group_key, field_val);
            }
            void update_open_group_quantity(cpp_redis::client &r, OpenGroup &open_group) {
                r.hmset(open_group.group_key, {
                        {group::CLOSE_UNIT_QUANTITY, std::to_string(open_group.close_unit_quantity)},
                        {group::CLOSING_UNIT_QUANTITY, std::to_string(open_group.closing_unit_quantity)}
                });
            }

            void update_open_group_after_trade(cpp_redis::client &r, OpenGroup &open_group) {
                std::vector<std::pair<std::string, std::string>> field_val = {
                        {group::AVG_COST, base::to_string(open_group.avg_cost)},
                        {group::TRANSACTION_COST, base::to_string(open_group.transaction_cost)}
                };
                for (int i = 0; i < leg_num_; ++i) {
                    field_val.insert(field_val.end(), {
                            {leg::LEG + std::to_string(i) + leg::AVG_PRICE, base::to_string(open_group.leg_items[i]->avg_price)},
                            {leg::LEG + std::to_string(i) + leg::TRANSACTION_COST, base::to_string(open_group.leg_items[i]->transaction_cost)}
                    });
                }
                r.hmset(open_group.group_key, field_val);
            }
            void update_close_group_after_trade(cpp_redis::client &r, CloseGroup &close_group) {
                std::vector<std::pair<std::string, std::string>> field_val = {
                        {group::AVG_COST, base::to_string(close_group.avg_cost)},
                        {group::TRANSACTION_COST, base::to_string(close_group.transaction_cost)}
                };
                for (int i = 0; i < leg_num_; ++i) {
                    field_val.insert(field_val.end(), {
                            {leg::LEG + std::to_string(i) + leg::AVG_PRICE, base::to_string(close_group.leg_items[i]->avg_price)},
                            {leg::LEG + std::to_string(i) + leg::TRANSACTION_COST, base::to_string(close_group.leg_items[i]->transaction_cost)}
                    });
                }
                r.hmset(close_group.group_key, field_val);
            }
            void update_close_group_pnl(cpp_redis::client &r, CloseGroup &close_group) {
                r.hmset(close_group.group_key, {
                        {group::REALIZED_PNL, base::to_string(close_group.realized_pnl)}
                });
            }
        private:
            int leg_num_;
            const static int LEG_COL_NUM = 5;
            std::vector<std::string> open_group_fields_;
            std::vector<std::string> close_group_fields_;
        };
    } /* dao  */

} /* iqt */

#endif /* end of include guard: _IQT_DAO_GROUP_DAO_HPP_ */
