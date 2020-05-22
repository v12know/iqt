//
// Created by carl on 17-11-13.
//
#include <iostream>
#include <chrono>
#include <gtest/gtest.h>

#include "base/util.hpp"
//#include "base/log.h"

using namespace std;


using std::chrono::system_clock;
static const auto one_day = std::chrono::hours(24);
static std::string now_date = base::strftimep(system_clock::now(), "%Y%m%d");
static bool switch_flag = false;

static std::string adjust_action_date(const std::string &src_action_date, const std::string &update_time,
                                      const system_clock::time_point &now) {
    std::string dest_action_date;
    if (update_time >= "23:55:00") {
//        auto now = system_clock::now();
        auto now_time = base::strftimep(now, "%H:%M:%S");
        if (now_time >= "23:50:00") {
            dest_action_date = std::move(base::strftimep(now, "%Y%m%d"));
        } else {
            auto yesterday = now - one_day;
/*                    if (!switch_flag) {
                        switch_flag = true;
                        now_date = base::strftimep(now, "%Y%m%d");
                    }*/
            dest_action_date = std::move(base::strftimep(yesterday, "%Y%m%d"));
        }
    } else if (update_time <= "00:05:00") {
//        auto now = system_clock::now();
        auto now_time = base::strftimep(now, "%H:%M:%S");
        if (now_time <= "00:10:00") {
            if (!switch_flag) {
                switch_flag = true;
                now_date = base::strftimep(now, "%Y%m%d");
            }
//                    dest_action_date = std::move(base::strftimep(now, "%Y%m%d"));
            dest_action_date = now_date;
        } else {// if (now_time >= "23:50:00")
            auto today = now + one_day;
/*                    if (!switch_flag) {
                        switch_flag = true;
                        now_date = base::strftimep(today, "%Y%m%d");
                    }*/
            dest_action_date = std::move(base::strftimep(today, "%Y%m%d"));
        }
    } else {
        dest_action_date = now_date;
    }
    return dest_action_date;
}

TEST(MiscTest, test0) {
    cout << "now_date: " << now_date << endl;
    auto now = base::strptimep("2018-03-31 23:59:59");
    auto action_date = adjust_action_date("", "23:59:59", now);
    cout << "action_date: " << action_date << ", now_date: " << now_date << endl;
    now = base::strptimep("2018-04-01 00:00:00");
    action_date = adjust_action_date("", "00:00:00", now);
    cout << "action_date: " << action_date << ", now_date: " << now_date << endl;
    now = base::strptimep("2018-04-01 00:04:22");
    action_date = adjust_action_date("", "00:04:59", now);
    cout << "action_date: " << action_date << ", now_date: " << now_date << endl;
    now = base::strptimep("2018-04-01 00:05:00");
    action_date = adjust_action_date("", "00:05:59", now);
    cout << "action_date: " << action_date << ", now_date: " << now_date << endl;
    now = base::strptimep("2018-04-01 00:10:00");
    action_date = adjust_action_date("", "00:10:59", now);
    cout << "action_date: " << action_date << ", now_date: " << now_date << endl;
    now = base::strptimep("2018-04-01 00:11:00");
    action_date = adjust_action_date("", "00:11:59", now);
    cout << "action_date: " << action_date << ", now_date: " << now_date << endl;
}
