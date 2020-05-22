//
// Created by carl on 17-12-14.
//

#include <gtest/gtest.h>
#include "base/log.h"
#include "base/redis.h"
#include "service/future_info_service.hpp"

using namespace iqt::service;
using namespace iqt::model;
using namespace base::redis;
using namespace std;
TEST(FutureInfoServiceTest, test0)
{
    FutureInfoService fis;
    auto r = RedisFactory::get_redis();
    std::unordered_map<std::string, std::shared_ptr<Commission>> comm_map;
    auto comm = std::make_shared<Commission>();
    comm->order_book_id = "rb1805";
    comm->commission_type = iqt::trade::COMMISSION_TYPE::BY_MONEY;
    comm->close_commission_ratio = 0.2;
    comm->open_commission_ratio = 0.2;
    comm->close_commission_today_ratio = 0.3;
    comm_map[comm->order_book_id] = comm;
    auto comm1 = std::make_shared<Commission>();
    *comm1 = *comm;
    comm1->order_book_id = "MA709";
    comm_map[comm1->order_book_id] = comm1;

    fis.add_commissions(*r, comm_map);
    r->sync_commit();
    auto res = fis.get_commissions(*r);
    for (auto &item : res) {
        cout << item.second->order_book_id << endl;
    }
}
