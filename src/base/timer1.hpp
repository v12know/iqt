#ifndef _BASE_TIMER1_HPP_
#define _BASE_TIMER1_HPP_

#include<functional>
#include<chrono>
#include<thread>
#include<atomic>
#include<memory>
#include<mutex>
#include<condition_variable>
namespace base {
	
using namespace std::chrono;

class Timer{
public:
	Timer() :expired_(true), tryToExpire_(false){
	}

	Timer(const Timer& t){
		expired_ = t.expired_.load();
		tryToExpire_ = t.tryToExpire_.load();
	}
	~Timer(){
		expire();
		//		std::cout << "timer destructed!" << std::endl;
	}

	void startTimer(int interval, std::function<void()> task){
		if (expired_ == false){
			//			std::cout << "timer is currently running, please expire it first..." << std::endl;
			return;
		}
		expired_ = false;
		std::thread([this, interval, task](){
			while (!tryToExpire_){
				std::this_thread::sleep_for(std::chrono::milliseconds(interval));
				task();
			}
			//			std::cout << "stop task..." << std::endl;
			{
				std::lock_guard<std::mutex> locker(mutex_);
				expired_ = true;
				expired_cond_.notify_one();
			}
		}).detach();
	}

	void expire(){
		if (expired_){
			return;
		}

		if (tryToExpire_){
			//			std::cout << "timer is trying to expire, please wait..." << std::endl;
			return;
		}
		tryToExpire_ = true;
		{
			std::unique_lock<std::mutex> locker(mutex_);
			expired_cond_.wait(locker, [this]{return expired_ == true; });
			if (expired_ == true){
				//				std::cout << "timer expired!" << std::endl;
				tryToExpire_ = false;
			}
		}
	}
    
	template<typename callable, class... arguments>
	void syncWait(int after, callable&& f, arguments&&... args){

		std::function<typename std::result_of<callable(arguments...)>::type()> task
			(std::bind(std::forward<callable>(f), std::forward<arguments>(args)...));
		std::this_thread::sleep_for(std::chrono::milliseconds(after));
		task();
	}
	template<typename callable, class... arguments>
	void asyncWait(int after, callable&& f, arguments&&... args){
		std::function<typename std::result_of<callable(arguments...)>::type()> task
			(std::bind(std::forward<callable>(f), std::forward<arguments>(args)...));

		std::thread([after, task](){
			std::this_thread::sleep_for(std::chrono::milliseconds(after));
			task();
		}).detach();
	}

	template<typename callable, class... arguments>
	void asyncWaitUtil(const system_clock::time_point &absTp, callable&& f, arguments&&... args){
		std::function<typename std::result_of<callable(arguments...)>::type()> task
			(std::bind(std::forward<callable>(f), std::forward<arguments>(args)...));

		std::thread([absTp, task](){
			std::this_thread::sleep_until(absTp);
			task();
		}).detach();
	}
	
private:
	std::atomic<bool> expired_;
	std::atomic<bool> tryToExpire_;
	std::mutex mutex_;
	std::condition_variable expired_cond_;
};

} /* base  */ 
#endif /* end of include guard: _BASE_TIMER1_HPP_ */

////////////////////test.cpp
/*#include<iostream>
#include<string>
#include<memory>
#include"timer.hpp"
using namespace std;
void EchoFunc(std::string&& s){
	std::cout << "test : " << s << endl;
}

int main(){
	Timer t;
	//周期性执行定时任务	
	t.startTimer(1000, std::bind(EchoFunc,"hello world!"));
	std::this_thread::sleep_for(std::chrono::seconds(4));
	std::cout << "try to expire timer!" << std::endl;
	t.expire();

	//周期性执行定时任务
	t.startTimer(1000, std::bind(EchoFunc,  "hello c++11!"));
	std::this_thread::sleep_for(std::chrono::seconds(4));
	std::cout << "try to expire timer!" << std::endl;
	t.expire();

	std::this_thread::sleep_for(std::chrono::seconds(2));

	//只执行一次定时任务
	//同步
	t.syncWait(1000, EchoFunc, "hello world!");
	//异步
	t.asyncWait(1000, EchoFunc, "hello c++11!");

	std::this_thread::sleep_for(std::chrono::seconds(2));

	return 0;
}*/
