#ifndef _IQT_CTP_MD_API_H_
#define _IQT_CTP_MD_API_H_

#include <string>
#include <vector>
#include <atomic>

#include "base/macro.hpp"
#include "ctp/ThostFtdcMdApi.h"
#include "trade/observer/md_subject.h"

namespace iqt {
    namespace trade {

        class CtpMdGateway;
/**
 * @synopsis 该类对ctp行情接口类（CThostFtdcMdApi）进行封装
 */
        class CtpMdApi : public CThostFtdcMdSpi {
            friend class CtpMdGateway;
        public:

            CtpMdApi(CtpMdGateway *gateway, const std::string &broker_id, const std::string &user_id, const std::string &password,
                      const std::string &address);

            virtual ~CtpMdApi();

            void join();

            void close();

            /**
             * @synopsis starytMdApi 开始行情接口
             */
            void connect();

            ///当客户端与交易后台建立起通信连接时（还未登录前），该方法被调用。
            void OnFrontConnected() override;

            ///当客户端与交易后台通信连接断开时，该方法被调用。当发生这个情况后，API会自动重新连接，客户端可不做处理。
            ///@param nReason 错误原因
            ///        0x1001 网络读失败
            ///        0x1002 网络写失败
            ///        0x2001 接收心跳超时
            ///        0x2002 发送心跳失败
            ///        0x2003 收到错误报文
            void OnFrontDisconnected(int nReason) override;
            ///心跳超时警告。当长时间未收到报文时，该方法被调用。
            ///@param nTimeLapse 距离上次接收报文的时间
            ///warning 目前此接口已经不再起效
//          void OnHeartBeatWarning(int nTimeLapse) override {};

            /**
            * @synopsis reqUserLogin 发送用户登录请求
            */
            void login();

            ///登录请求响应
            void
            OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                           bool bIsLast) override;

            /**
             * @synopsis reqUserLogout 发送用户登出请求
             */
            void logout();

            ///登出请求响应
            void
            OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                            bool bIsLast) override;

            /**
             * @synopsis subscribe 发送订阅行情数据请求
             */
            void subscribe(const std::vector<std::string> &subscribed_vec);


            ///订阅行情应答
            void
            OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo,
                               int nRequestID, bool bIsLast) override;

            /**
             * @synopsis unsubscribe 发送取消订阅行情数据请求
             */
            void unsubscribe(const std::vector<std::string> &ins_id_vec);

            ///取消订阅行情应答
            void OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument,
                                      CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

            ///深度行情通知
            void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData) override;

            ///错误应答
            void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

            int next_req_id() {
                return ++req_id_;
            }

        private:
            /* data */
            CThostFtdcMdApi *md_api_;//ctp行情接口类
            CtpMdGateway *gateway_;
            std::string broker_id_;//经纪公司代码
            std::string user_id_;//用户代码
            std::string password_;//密码
            std::string address_;//前置机IP地址
            std::atomic_int req_id_{0};

            bool connected_ = false;
            bool logged_in_ = false;
        };


    } /* ctp */
} /* iqt */


#endif /* end of include guard: _IQT_CTP_MD_API_H_ */
