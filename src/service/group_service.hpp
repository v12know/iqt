#ifndef _IQT_SERVICE_GROUP_SERVICE_HPP_
#define _IQT_SERVICE_GROUP_SERVICE_HPP_

#include "model/group.h"
#include "dao/group_dao.hpp"
//#include "base/redis.h"


namespace iqt {
/*    namespace cpp_redis {
        class client;
    }*/
    namespace service {
//    using namespace base::redis;
        using namespace model;
        using namespace dao;

        class GroupService {
        public:
            GroupService(const std::string &log_name, int leg_num) : log_name_(log_name), leg_num_(leg_num), group_dao_(leg_num_) {}

            OpenGroupPtr get_open_group(cpp_redis::client &r, const std::string &group_key) {
                OpenGroupPtr open_group = nullptr;
                auto iter = open_group_map_.find(group_key);
                if (iter != open_group_map_.cend()) {
                    open_group = iter->second;
                    log_trace_g(log_name_, "get open_group from cache: {0}", open_group->to_string());
                } else {
                    open_group = group_dao_.get_open_group(r, group_key);
//                PPK_ASSERT_ERROR(open_group, "can't find open_group on redis, group_key=%s", group_key.c_str());
                    assert(open_group);
                    log_trace_g(log_name_, "get open_group from redis: {0}", open_group->to_string());
                    open_group_map_[group_key] = open_group;
                }
                return open_group;
            }

            CloseGroupPtr get_close_group(cpp_redis::client &r, const std::string &group_key) {
                CloseGroupPtr close_group = nullptr;
                auto iter = close_group_map_.find(group_key);
                if (iter != close_group_map_.cend()) {
                    close_group = iter->second;
                    log_trace_g(log_name_, "get close_group from cache: {0}", close_group->to_string());
                } else {
                    close_group = group_dao_.get_close_group(r, group_key);
                    assert(close_group);
                    log_trace_g(log_name_, "get close_group from redis: {0}", close_group->to_string());
//                PPK_ASSERT_ERROR(close_group, "can't find close_group on redis, group_key=%s", group_key.c_str());
                    for (auto &open_group : close_group->open_groups) {
                        get_open_group(r, open_group);
                    }
                    close_group_map_[group_key] = close_group;
                }
                return close_group;
            }

            //TODO没有加载leg_items.orders
            void get_open_group(cpp_redis::client &r, OpenGroupPtr &open_group) {//open_group的引用符号不能去掉
                auto iter = open_group_map_.find(open_group->group_key);
                if (iter != open_group_map_.cend()) {
                    open_group = iter->second;
                    log_trace_g(log_name_, "get open_group from cache: {0}", open_group->to_string());
                } else {
                    group_dao_.get_open_group(r, *open_group);
                    assert(open_group);
                    log_trace_g(log_name_, "get open_group from redis: {0}", open_group->to_string());
//                PPK_ASSERT_ERROR(open_group, "can't find open_group on redis, group_key=%s", open_group->group_key.c_str());
                    open_group_map_[open_group->group_key] = open_group;
                }
            }

            void add_open_group(cpp_redis::client &r, OpenGroupPtr open_group) {
                open_group->group_key = group::OPEN_GROUP + std::to_string(group_dao_.incr(r, group::GROUP));
                open_group_map_[open_group->group_key] = open_group;
                group_dao_.add_open_group(r, *open_group);
            }

            void add_close_group(cpp_redis::client &r, CloseGroupPtr close_group) {
                close_group->group_key = group::CLOSE_GROUP + std::to_string(group_dao_.incr(r, group::GROUP));
                close_group_map_[close_group->group_key] = close_group;
                group_dao_.add_close_group(r, *close_group);
            }

            void update_open_group(cpp_redis::client &r, OpenGroup &open_group) {
                group_dao_.update_open_group(r, open_group);
            }

            void update_close_group(cpp_redis::client &r, CloseGroup &close_group) {
                group_dao_.update_close_group(r, close_group);
            }

            void update_open_group_quantity(cpp_redis::client &r, OpenGroup &open_group) {
                group_dao_.update_open_group_quantity(r, open_group);
            }

            void update_open_group_after_trade(cpp_redis::client &r, OpenGroup &open_group) {
                group_dao_.update_open_group_after_trade(r, open_group);
            }

            void update_close_group_after_trade(cpp_redis::client &r, CloseGroup &close_group) {
                group_dao_.update_close_group_after_trade(r, close_group);
            }

            void update_close_group_pnl(cpp_redis::client &r, CloseGroup &close_group) {
                group_dao_.update_close_group_pnl(r, close_group);
            }

//        void add_

        private:
            std::string log_name_;
            int leg_num_;
            GroupDao group_dao_;
            std::unordered_map<std::string, OpenGroupPtr> open_group_map_;
            std::unordered_map<std::string, CloseGroupPtr> close_group_map_;
        };

    } /* service */
} /* iqt */
#endif /* end of include guard: _IQT_SERVICE_GROUP_SERVICE_HPP_ */
