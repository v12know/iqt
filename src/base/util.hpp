#ifndef _BASE_UTIL_HPP_
#define _BASE_UTIL_HPP_

#include <string>
#include <vector>
#include <chrono>


namespace base {

//    using namespace std;

    std::vector<std::string> split(std::string_view str, std::string_view pattern);

    template<class T>
    size_t getArrLen(T &array);

    template<typename Enumeration>
    auto enum2index(const Enumeration value)
    -> typename std::underlying_type<Enumeration>::type {
        return static_cast<typename std::underlying_type<Enumeration>::type>(value);
    }

    using std::chrono::system_clock;

    std::string strftimep(system_clock::time_point timep = system_clock::now(),
                          std::string_view format = "%Y-%m-%d %H:%M:%S");

    system_clock::time_point strptimep(std::string_view dt_str,
                                              std::string_view format
                                              = "%Y-%m-%d %H:%M:%S");

    /***
     * 获取毫秒分部
     * @param timep c++ time_point 类型
     * @return 毫秒分部
     */
    long ms(const system_clock::time_point &timep = system_clock::now());

    /***
     * 获取微秒分部
     * @param timep c++ time_point 类型
     * @return 微秒分部
     */
    long us(const system_clock::time_point &timep = system_clock::now());

    /***
     * 获取秒级别的时间戳
     * @param timep c++ time_point 类型
     * @return 秒级别的时间戳
     */
    long timestamp(const system_clock::time_point &timep = system_clock::now());

    /***
     * 获取毫秒级别的时间戳
     * @param timep c++ time_point 类型
     * @return 毫秒级别的时间戳
     */
    long long timestamp_ms(const system_clock::time_point &timep = system_clock::now());

    /***
     * 获取微秒级别的时间戳
     * @param timep c++ time_point 类型
     * @return 微秒级别的时间戳
     */
    long long timestamp_us(const system_clock::time_point &timep = system_clock::now());

    //从字符串开始处依次替换子字符串
    std::string &replace_all(std::string &str, const std::string &old_value, const std::string &new_value);

    std::string replace_all(const std::string &str, const std::string &old_value, const std::string &new_value);

    /***
     * 将gbk转换为utf8
     * from: https://www.zhihu.com/question/39186934/answer/80443490
     * @param gbk_str gbk字符串
     * @return 转换好的utf8字符串
     */
//    std::string gbk2utf(const std::string &gbk_str);
    /***
     * 将utf8转换为gbk
     * from: https://www.zhihu.com/question/39186934/answer/80443490
     * @param utf8_str utf8字符串
     * @return 转换好的gbk字符串
     */
//    std::string utf2gbk(const std::string &utf8_str);

    /***
     * 将浮点类型转换为字符串
     * 该函数主要针对浮点型转换成字符串会多出很多0的情形
     * @param digit
     * @return
     */
    std::string to_string(double digit);

} /* base */


#include "util_impl.hpp"

#endif /* end of include guard: _BASE_UTIL_HPP_ */

