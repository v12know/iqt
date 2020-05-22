#ifndef _BASE_LOG_H_
#define _BASE_LOG_H_

#include <memory>
#include <sstream>

//由于在对以下宏展开时需要用到spdlog的定义，故不能用前置声明来代替引入头文件
#include "spdlog/spdlog.h"

#include "base/macro.hpp"

//定义日志等级的宏 start
#define log_tmpl(LOG_NAME, LEVEL, fmt, ...) \
do { \
    std::ostringstream __oss__; \
    __oss__ << "[" << __FILE__ << ":" << __LINE__ << "] " << (fmt); \
    ::base::LogFactory::get_logger(LOG_NAME)->LEVEL(__oss__.str().c_str(), ##__VA_ARGS__); \
} while(false)
#ifndef NDEBUG
#define log_trace_g(LOG_NAME, fmt, ...) log_tmpl(LOG_NAME, trace, (fmt), ##__VA_ARGS__)
#else
#define log_trace_g(fmt, ...)
#endif
#define log_trace(fmt, ...) log_trace_g(base::LogFactory::get_def_log_name(), (fmt), ##__VA_ARGS__)

#ifndef NDEBUG
#define log_debug_g(LOG_NAME, fmt, ...) log_tmpl(LOG_NAME, debug, (fmt), ##__VA_ARGS__)
#else
#define log_debug_g(fmt, ...)
#endif
#define log_debug(fmt, ...) log_debug_g(base::LogFactory::get_def_log_name(), (fmt), ##__VA_ARGS__)

#ifndef NDEBUG
#define log_info_g(LOG_NAME, fmt, ...) log_tmpl(LOG_NAME, info, (fmt), ##__VA_ARGS__)
#else
#define log_info_g(fmt, ...)
#endif
#define log_info(fmt, ...) log_info_g(base::LogFactory::get_def_log_name(), (fmt), ##__VA_ARGS__)

#define log_warn_g(LOG_NAME, fmt, ...) log_tmpl(LOG_NAME, warn, (fmt), ##__VA_ARGS__)
#define log_warn(fmt, ...) log_warn_g(base::LogFactory::get_def_log_name(), (fmt), ##__VA_ARGS__)

#define log_error_g(LOG_NAME, fmt, ...) log_tmpl(LOG_NAME, error, (fmt), ##__VA_ARGS__)
#define log_error(fmt, ...) log_error_g(base::LogFactory::get_def_log_name(), (fmt), ##__VA_ARGS__)

#define log_critical_g(LOG_NAME, fmt, ...) log_tmpl(LOG_NAME, critical, (fmt), ##__VA_ARGS__)
#define log_critical(fmt, ...) log_critical_g(base::LogFactory::get_def_log_name(), (fmt), ##__VA_ARGS__)
//定义日志等级的宏 end


// Forward declarations start.
namespace spdlog {
    class logger;
}
// Forward declarations end.

namespace base {

    struct LogConf {
        std::string level = "info";
        std::string base_dir = "../log";
        std::string def_log_name = "default";
        size_t async_size = 4096;
        bool console_flag = true;

        LogConf() = default;

        LogConf(const std::string &_level, const std::string &_base_dir, const std::string &_def_log_name,
                const size_t &_async_size, const bool _console_flag) :
                level(_level), base_dir(_base_dir), def_log_name(_def_log_name), async_size(_async_size),
                console_flag(_console_flag) {}
    };

/*    class Log {
    public:
        std::shared_ptr<spdlog::logger> getLogger() {
            return combinedLogger_;
        }

        Log(const std::string &log_name, LogConf *log_conf);

        ~Log();

        //Log(Log const &config) = delete;
        //Log & operator=(Log const &config) = delete;

        void init();

        //template <typename... Args> void info(const char *fileName, const int fileNum, const char *fmt, const Args&... args);
        //template <typename... Args> void info(const char *fmt, const Args&... args);
        //template <typename... Args>
        //inline void info(const char *fileName, const int fileNum, const char *fmt, const Args&... args) {
        //std::ostringstream oss;
        //oss << "[" << fileName << ":" << fileNum << "] " << fmt;
        //Log::instance()->combinedLogger_->info(oss.str().c_str(), args...);
        //}

    private:
        std::shared_ptr<spdlog::logger> combinedLogger_;
        std::string log_name_;
        LogConf *log_conf_;
    };*/


    class LogFactory {
    public:
        static LogFactory *instance();

        static void
        init(const std::string &_level, const std::string &_base_dir, const std::string &_def_log_name,
                          const size_t &_async_size, const bool _console_flag);

        static std::string &get_def_log_name() {
            return LogFactory::instance()->log_conf_->def_log_name;
        }

        static void create_logger(const std::string &log_name, const std::string &log_level = "");

        static std::shared_ptr<spdlog::logger> get_logger(const std::string &log_name);

        static void flush_all();

        DISALLOW_COPY_AND_ASSIGN(LogFactory);

    private:
        LogFactory() = default;

        std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> log_map_;
        LogConf *log_conf_;
        static LogFactory *s_log_factory;
    };

    inline std::shared_ptr<spdlog::logger> LogFactory::get_logger(const std::string &log_name) {
        return instance()->log_map_.at(log_name);
    }
}/* base */


#endif /* end of include guard: _BASE_LOG_H_ */
