#include "log.h"

//#include <mutex>
#include <iostream>
#include <iterator>

namespace base {

    using std::cout;
    using std::cerr;
    using std::endl;


    static const char *skLevelNameArr[]{"trace", "debug", "info", "warn", "error", "critical", "off"};
    static constexpr size_t skLevelNameArrLen = sizeof(skLevelNameArr) / sizeof(skLevelNameArr[0]);

    static inline spdlog::level::level_enum toLevelEnum(const std::string &level) {
        for (size_t i = 0; i != skLevelNameArrLen; ++i) {
            if (level.compare(skLevelNameArr[i]) == 0) {
                return static_cast<spdlog::level::level_enum>(i);
            }
        }
        cerr << "Can't match any level name, will use trace level!" << endl;
        return spdlog::level::trace;
    }

    LogFactory *LogFactory::s_log_factory;

    LogFactory *LogFactory::instance() {//不能在头文件实现，否则作为动态链接库使用会出现指针指向不一致问题
        static std::once_flag oc;//用于call_once的局部静态变量
        std::call_once(oc, [&] { s_log_factory = new LogFactory(); });
        return s_log_factory;
    }

    void LogFactory::create_logger(const std::string &log_name, const std::string &log_level) {
        cout << "LogFactory::create_logger: log_name=" << log_name << endl;
        auto factory = LogFactory::instance();
        auto log_conf = factory->log_conf_;

        std::vector<spdlog::sink_ptr> sinks;

        if (log_conf->console_flag == true) {
            auto consoleSink = std::make_shared<spdlog::sinks::stdout_sink_st>();
            sinks.push_back(consoleSink);
        }
        // spdlog will create a queue with **pre-allocated** slots per each logger
        if (log_conf->async_size > 0) {
            spdlog::set_async_mode(log_conf->async_size, spdlog::async_overflow_policy::block_retry,
                                   nullptr, std::chrono::seconds(10));
        }
        using namespace spdlog::sinks;
        using namespace spdlog::details;
        std::string full_name = log_conf->base_dir + "/" + log_name;
        auto asyncSink = std::make_shared<daily_file_sink<null_mutex, dateonly_daily_file_name_calculator>>(full_name, 0, 0);
        sinks.push_back(asyncSink);

        auto combined_logger = std::make_shared<spdlog::logger>(log_name, std::begin(sinks), std::end(sinks));
        combined_logger->flush_on(spdlog::level::warn);

        spdlog::register_logger(combined_logger);
        spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [thread %t] [%l] %v")oChange log level to all loggers to warning and above
        //spdlog::set_level(spdlog::level::info);
        if (log_level == "") {
            spdlog::level::level_enum levelEnum = toLevelEnum(log_conf->level);
            spdlog::set_level(levelEnum);
        } else {
            spdlog::level::level_enum levelEnum = toLevelEnum(log_level);
            spdlog::set_level(levelEnum);
        }
//		log_map_.emplace(log_name_, *this);
//		log_map_[log_name_] = *this;
        factory->log_map_[log_name] = combined_logger;
    }


    void LogFactory::flush_all() {
        auto factory = LogFactory::instance();
        for (auto item : factory->log_map_) {
            item.second->flush();
        }
    }

    void LogFactory::init(const std::string &_level, const std::string &_base_dir,
                                       const std::string &_def_log_name, const size_t &_async_size,
                                       const bool _console_flag) {
        auto factory = LogFactory::instance();
        factory->log_conf_ = new LogConf(_level, _base_dir, _def_log_name, _async_size, _console_flag);
        factory->create_logger(_def_log_name);
    }


//    std::string &LogFactory::get_def_log_name() {
//        return LogFactory::instance()->log_conf_->def_log_name;
//    }
} /* base */