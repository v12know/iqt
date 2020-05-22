#ifndef _IQT_GLOBAL_H_
#define _IQT_GLOBAL_H_

#include <string>
#include <ctime>

#include "base/util.hpp"
//#include "model/instrument.h"
/**
 * @file global.h
 * @synopsis 集中存放全局变量和函数
 * @author Carl, v12know@hotmail.com
 * @version 1.0.0
 * @date 2017-01-09
 */
namespace iqt {
//    namespace model {
//        class Tick;
//    }

    namespace global {

/////////////////////////macros/////////////////////////////////////////
//#define CONVERT_CTP_STR(src_str) base::gbk2utf(src_str)
#define CONVERT_CTP_STR(src_str) std::string(src_str)
/////////////////////////variables/////////////////////////////////////////
//extern std::atomic_int gRequestId; //全局共享请求ID

//        extern const unsigned RETRY_MAX;//失败重连最大次数

////////////////////////functions/////////////////////////////////////////


        /**
         * @synopsis mapDisconReasonVal 将CTP前置断开原因值映射为字符串
         *
         * @param reason 原因值
         *
         * @returns 对应字符串
         */
        inline std::string mapDisconReasonVal(const int &reason) {
            std::string reasonStr;
            switch (reason) {
                case 0x1001:
                    reasonStr = "网络读失败";
                    break;
                case 0x1002:
                    reasonStr = "网络写失败";
                    break;
                case 0x2001:
                    reasonStr = "接收心跳超时";
                    break;
                case 0x2002:
                    reasonStr = "发送心跳失败";
                    break;
                case 0x2003:
                    reasonStr = "收到错误报文";
                    break;
                default:
                    reasonStr = "未知错误";
            }
            return reasonStr;
        }

        /**
         * @synopsis mapRetVal 将请求返回值映射为字符串
         *
         * @param ret 请求返回值
         *
         * @returns 对应字符串
         */
        inline std::string mapRetVal(const int &ret) {
            std::string retStr;
            switch (ret) {
                case -1:
                    retStr = "因网络原因发送失败";
                    break;
                case -2:
                    retStr = "未处理请求队列总数量超限";
                    break;
                case -3:
                    retStr = "每秒发送请求数量超限";
                    break;
                default:
                    retStr = "未知错误";
            }
            return retStr;
        }


        double margin_of(const std::string &order_book_id, int quantity, double price);

        char get_year3(const std::string & instrument_id);

/*        inline std::string make_order_book_id(const std::string &instrument_id) {
            return instrument_id;
        }*/
        inline std::string make_order_book_id(const std::string &instrument_id) {
            auto size = instrument_id.size();
            if (size < 4) {
                return instrument_id;
            }
            char year3 = get_year3(instrument_id);
            auto pos = size - 4;
            std::string order_book_id;
            for (decltype(size) i = 0; i < size; ++i) {
                order_book_id.append(1, toupper(instrument_id[i]));
                if (i == pos && !isdigit(instrument_id[i])) {
                    order_book_id.append(1, year3);
                }
            }
            return order_book_id;
        }


    } /* global */

} /* iqt */
#endif /* end of include guard: _IQT_GLOBAL_H_ */
