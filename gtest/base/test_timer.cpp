//
// Created by carl on 17-11-13.
//
#include <iostream>
#include <gtest/gtest.h>
#include "base/timer.hpp"
//#include "base/log.h"

using namespace std;
TEST(TimerTest, test0)
{
    base::Timer timer;
    auto start = std::chrono::system_clock::now();
    timer.schedule(100, [&](void *){
        auto end = std::chrono::system_clock::now();
        cout << "schedule100: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << endl;
    });
    timer.schedule(50, [&](void *){
        auto end = std::chrono::system_clock::now();
        cout << "schedule50: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << endl;
    });
    timer.schedule(200, [&](void *){
        auto end = std::chrono::system_clock::now();
        cout << "schedule200: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << endl;
    });
    timer.schedule(0, [&](void *){
        auto end = std::chrono::system_clock::now();
        cout << "schedule0: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << endl;
    });
    int i = 0;
    timer.schedule(200, 1000, [&](void *){
        if (i >= 10) {
            timer.stop();
        }
        auto end = std::chrono::system_clock::now();
        cout << "schedule200 interval1000: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << endl;
        ++i;
    });
    int j = 0;
    timer.schedule(0, 500, [&](void *){

//        if (j >= 5) {
//            timer.stop();
//        }
        auto end = std::chrono::system_clock::now();
        cout << "schedule0 interval500: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << endl;
        ++j;
    });
    timer.join();
}
