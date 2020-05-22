#ifndef _BASE_UTIL_IMPL_HPP_
#define _BASE_UTIL_IMPL_HPP_

#include "util.hpp"

#include <cstring>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <memory>
#include <locale>
//#include <codecvt>
//#include <cwchar>

#include "base/exception.h"
#include "base/milo/dtoa_milo.h"

namespace base {

    inline std::vector<std::string> split(const std::string_view str, const std::string_view pattern) {
        size_t start = 0, pos;
        std::vector<std::string> result_vec;
        while ((pos = str.find(pattern, start)) != std::string_view::npos) {
            result_vec.emplace_back(str.substr(start, pos - start));
            start = pos + pattern.size();
        }
        result_vec.emplace_back(str.substr(start));
        return result_vec;
    }

    template<class T>
    inline size_t getArrLen(T &array) {
        return (sizeof(array) / sizeof(array[0]));
    }

    inline std::string strftimep(system_clock::time_point timep,
                          std::string_view format) {
        std::time_t t = system_clock::to_time_t(timep);
        std::tm* ptm  = std::localtime(&t);
        char dt_str[128];
        if (!std::strftime(dt_str,
                                    sizeof(dt_str),
                                    format[format.size()] == '\0'
                                    ? format.data()
                                    : std::string(dt_str).data(),
                                    ptm)) {
            MY_THROW(base::BaseException, "format time_point to datetime string using format["
                        + std::string(format) + "] error!");
        }
        // std::string timeStr = std::put_time(ptm, format);
        return std::string(dt_str);
    }
    inline system_clock::time_point strptimep(std::string_view dt_str,
                                       std::string_view format) {
        std::tm tm;
        if (!strptime(
                dt_str[dt_str.size()] == '\0' ? dt_str.data()
                                              : std::string(dt_str).data(),
                format[format.size()] == '\0' ? format.data()
                                              : std::string(dt_str).data(),
                &tm)) {
            MY_THROW(base::BaseException, "format time_point from datetime string["
                        + std::string(dt_str) + "] using format["
                        + std::string(format) + "] error!");
        }
        std::time_t t = std::mktime(&tm);
        return system_clock::from_time_t(t);
    }

    inline long ms(const system_clock::time_point &timep) {
        auto tp_s = std::chrono::time_point_cast<std::chrono::seconds>(timep);
        return std::chrono::duration_cast<std::chrono::milliseconds>(timep.time_since_epoch()).count() - std::chrono::duration_cast<std::chrono::milliseconds>(tp_s.time_since_epoch()).count();
    }

    inline long us(const system_clock::time_point &timep) {
        auto tp_s = std::chrono::time_point_cast<std::chrono::seconds>(timep);
        return std::chrono::duration_cast<std::chrono::microseconds>(timep.time_since_epoch()).count() - std::chrono::duration_cast<std::chrono::milliseconds>(tp_s.time_since_epoch()).count();
    }

    inline long timestamp(const system_clock::time_point &timep) {
        return std::chrono::duration_cast<std::chrono::seconds>(timep.time_since_epoch()).count();
    }

    inline long long timestamp_ms(const system_clock::time_point &timep) {
        return std::chrono::duration_cast<std::chrono::milliseconds>(timep.time_since_epoch()).count();
    }

    inline long long timestamp_us(const system_clock::time_point &timep) {
        return std::chrono::duration_cast<std::chrono::microseconds>(timep.time_since_epoch()).count();
    }

    //从字符串开始处依次替换子字符串
    inline std::string &replace_all(std::string &str, const std::string &old_value, const std::string &new_value) {
        //const char* convert to char*
        //char *str_c = new char[strlen(str.c_str()) + 1];
        //strcpy(str_c, str.c_str());
        char *str_c = const_cast<char *>(str.c_str());
        std::string ts;
        char *save_ptr = nullptr;
        char *tmp_str = strtok_r(str_c, old_value.c_str(), &save_ptr);
        while (tmp_str) {
            ts.append(std::string(tmp_str));
            tmp_str = strtok_r(nullptr, old_value.c_str(), &save_ptr);
            if (tmp_str) {
                ts.append(new_value);
            }
        }
        //delete[] str_c;
        str = std::move(ts);

        return str;
    }

    inline std::string replace_all(const std::string &str, const std::string &old_value, const std::string &new_value) {
        //const char* convert to char*
        char *str_c = new char[strlen(str.c_str()) + 1];
        strcpy(str_c, str.c_str());
//        char *str_c = const_cast<char *>(str.c_str());
        std::string ts;
        char *save_ptr = nullptr;
        char *tmp_str = strtok_r(str_c, old_value.c_str(), &save_ptr);
        while (tmp_str) {
            ts.append(std::string(tmp_str));
            tmp_str = strtok_r(nullptr, old_value.c_str(), &save_ptr);
            if (tmp_str) {
                ts.append(new_value);
            }
        }
        delete[] str_c;
//        str = std::move(ts);

        return std::move(ts);
    }
/*
#ifdef WIN32
    static const char *GBK_LOCALE_NAME = ".936"; //GBK在windows下的locale name
#else
    static const char *GBK_LOCALE_NAME = "zh_CN.GBK"; //GBK在Linux下的locale name
#endif
    // utility wrapper to adapt locale-bound facets for wstring/wbuffer convert
    template<class Facet>
    struct deletable_facet : Facet
    {
        template<class ...Args>
        deletable_facet(Args&& ...args) : Facet(std::forward<Args>(args)...) {}
        ~deletable_facet() {}
    };
    typedef deletable_facet<std::codecvt_byname<wchar_t, char, std::mbstate_t>> gbfacet_t;

    inline std::string gbk2utf(const std::string &gbk_str) {
//        std::string gbk_str {"\xCC\xCC"};  //0xCCCC，"烫"的GBK码

//构造GBK与wstring间的转码器（wstring_convert在析构时会负责销毁codecvt_byname，所以不用自己delete）
        std::wstring_convert<gbfacet_t> cv1(
                new gbfacet_t(GBK_LOCALE_NAME));
        std::wstring tmp_wstr = cv1.from_bytes(gbk_str);

        std::wstring_convert<std::codecvt_utf8<wchar_t>> cv2;
        return cv2.to_bytes(tmp_wstr);
    }

    inline std::string utf2gbk(const std::string &utf8_str) {
//        std::string gbk_str {"\xE7\x83\xAB"};  //utf8_str里的内容应该是"\xE7\x83\xAB"（烫的UTF8）
        std::wstring_convert<std::codecvt_utf8<wchar_t>> cv2;
        std::wstring tmp_wstr = cv2.from_bytes(utf8_str);

//构造GBK与wstring间的转码器（wstring_convert在析构时会负责销毁codecvt_byname，所以不用自己delete）
        std::wstring_convert<gbfacet_t> cv1(
                new gbfacet_t(GBK_LOCALE_NAME));

        return cv1.to_bytes(tmp_wstr);
    }*/

    inline std::string to_string(double digit) {
        char buf[32 + 1];
        dtoa_milo(digit, buf);
        return std::string(buf);
    }
} /* base */

#endif /* end of include guard: _BASE_UTIL_IMPL_HPP_ */
