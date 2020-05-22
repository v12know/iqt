#ifndef _BASE_TIME_SPAN_HPP_
#define _BASE_TIME_SPAN_HPP_

#include<chrono>

#include "base/util.hpp"
#include "base/timer1.hpp"
namespace base {
	
using namespace std::chrono;

class TimeSpanSchduler{
public:
	TimeSpanSchduler(const std::string appName, const std::string &timeSpans) {
		appName_ = appName;
		//std::string timeSpans = "09:29:00~10:59:30,13:29:00~14:59:30,20:59:00~00:59:30";
		for (auto &validTimeSpan : base::split(timeSpans, ",")) {
			std::vector<std::string> pairVec = base::split(validTimeSpan, "~");
			validTimeSpanVec_.emplace_back(std::move(pairVec[0]), std::move(pairVec[1]));
		}
		std::sort(validTimeSpanVec_.begin(), validTimeSpanVec_.end(), [](const std::pair<std::string, std::string> &a, const std::pair<std::string, std::string> &b) {
			return a.first < b.first;
		});
	}

	void setSecondDiff(const long &secondDiff) {
		secondDiff_ = seconds(secondDiff);
	}
	//~TimeSpanSchduler() {}
	void schNextTime() {
		if (!validTimeSpanVec_.size()) {
			log_warn("[appName={0}] no limit valid time span!", appName_);
			validTimeFlag = true;
			return;
		}
		using std::chrono::system_clock;
		using std::chrono::seconds;
		//seconds secDiff = seconds(global::getSecondDiff());
		system_clock::time_point now = system_clock::now() + secondDiff_;
		std::string nowtimeStr = base::strftimep(now, "%H:%M:%S");
		std::string schTimeStr;
		size_t size = validTimeSpanVec_.size();
		//log_trace("nowtimeStr={0}, validTimeSpanVec_[last]={1}", nowtimeStr, validTimeSpanVec_[size - 1].second);
		if (validTimeSpanVec_[size - 1].second > nowtimeStr) {
			schTimeStr = validTimeSpanVec_[size - 1].second;
			validTimeFlag = true;
		} else {
			bool watchFlag = false;
			for (size_t i = 0; i < size; ++i) {
				if (validTimeSpanVec_[i].first > nowtimeStr) {
					schTimeStr = validTimeSpanVec_[i].first;
		//log_trace("schTimeStr={0}, i={1}", schTimeStr, i);
					validTimeFlag = false;
					watchFlag = true;
					break;
				}
				if (validTimeSpanVec_[i].second > nowtimeStr) {
					schTimeStr = validTimeSpanVec_[i].second;
		//log_trace("schTimeStr={0}, i={1}", schTimeStr, i);
					validTimeFlag = true;
					watchFlag = true;
					break;
				}
			}
			if (!watchFlag) {
				if (validTimeSpanVec_[size - 1].second < validTimeSpanVec_[0].first) {
					schTimeStr = validTimeSpanVec_[size - 1].second;
		//log_trace("schTimeStr={0}", schTimeStr);
					validTimeFlag = true;
				} else {
					schTimeStr = validTimeSpanVec_[0].first;
		//log_trace("schTimeStr={0}", schTimeStr);
					validTimeFlag = false;
				}
			}
		}
		std::string schDatetimeStr = base::strftimep(now + std::chrono::hours(schTimeStr < nowtimeStr ? 24 : 0), "%Y-%m-%d") + " " + schTimeStr;
		log_warn("[appName={0}] schedule next test valid time span is {1}, validTimeFlag={2}============", appName_, schDatetimeStr, validTimeFlag);

		timer_.asyncWaitUtil(base::strptimep(schDatetimeStr) - secondDiff_, std::bind(&TimeSpanSchduler::schNextTime, this));
	}

public:
	volatile bool validTimeFlag = false;
private:
	std::string appName_;
	std::vector<std::pair<std::string, std::string>> validTimeSpanVec_;
	base::Timer timer_;
	seconds secondDiff_;
};

} /* base  */ 
#endif /* end of include guard: _BASE_TIME_SPAN_HPP_ */

