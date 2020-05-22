#ifndef _IQT_SERVICE_FUTURE_INFO_SERVICE_HPP_
#define _IQT_SERVICE_FUTURE_INFO_SERVICE_HPP_

#include <vector>
#include <sstream>
#include <cereal/cereal.hpp>
#include <cereal/archives/json.hpp>
#include <iostream>

#include "model/future_info.h"
#include "dao/future_info_dao.hpp"

namespace iqt {
    namespace service {
//    using namespace base::redis;
        using namespace model;
        using namespace dao;

        class FutureInfoService {
        public:
            void add_commissions(cpp_redis::client &r,
                                 std::unordered_map<std::string, std::shared_ptr<model::Commission>> &comm_map) {
                std::vector<std::string> comm_vec;
                for (auto &item : comm_map) {
                    std::stringstream ss;
                    {
                        cereal::JSONOutputArchive archive(ss);
//                    cereal::XMLOutputArchive archive(ss);
                        item.second->serialize(archive);
//                        archive(*item.second);
                    }
                    std::cout << ss.str() << std::endl;
                    comm_vec.push_back(ss.str());
                }
                future_info_dao_.add_commissions(r, commission::COMMISSION_LIST, comm_vec);
                future_info_dao_.expire(r, commission::COMMISSION_LIST, 60 * 60 * 24);
            }

            std::unordered_map<std::string, std::shared_ptr<model::Commission>>
            get_commissions(cpp_redis::client &r) {
                auto comm_vec = future_info_dao_.get_commissions(r, commission::COMMISSION_LIST);
                std::unordered_map<std::string, std::shared_ptr<model::Commission>> comm_map;
                for (auto &comm_str : comm_vec) {
                    std::stringstream ss(comm_str);
                    auto comm = std::make_shared<Commission>();
                    {

                        cereal::JSONInputArchive archive(ss);
//                    cereal::XMLInputArchive archive(ss);
                        comm->serialize(archive);
//                    archive(*comm);
                    }
                    comm_map[comm->order_book_id] = comm;
                }
                return comm_map;
            }

            void set_commission_date(cpp_redis::client &r, const std::string &date) {
                future_info_dao_.set(r, commission::COMMISSION_DATE, date);
                future_info_dao_.expire(r, commission::COMMISSION_DATE, 60 * 60 * 24);
            }

            bool check_update_commission_date(cpp_redis::client &r, const std::string &now_date) {
                if (future_info_dao_.exists(r, {commission::COMMISSION_DATE})) {
                    std::string rec_date = future_info_dao_.get(r, commission::COMMISSION_DATE);
                    if (rec_date < now_date) {
                        return true;
                    } else {
                        return false;
                    }
                } else {
                    return true;
                }
            }

        private:
            FutureInfoDao future_info_dao_;
        };

    } /* service */
} /* iqt */
#endif /* end of include guard: _IQT_SERVICE_FUTURE_INFO_SERVICE_HPP_ */
