#include "ctp/md_api.h"

#include <algorithm>

#include "base/log.h"
#include "base/util.hpp"
#include "global.h"
#include "model/tick.h"
#include "md_gateway.h"
#include "base/raii.hpp"
#include "env.h"

namespace iqt {
    namespace trade {

        CtpMdApi::CtpMdApi(CtpMdGateway *gateway, const std::string &broker_id, const std::string &user_id,
                           const std::string &password,
                           const std::string &address) :
                gateway_(gateway), broker_id_(broker_id), user_id_(user_id),
                password_(password), address_(address) {

            log_warn("CtpMdApi constructor");
            //log_trace("instrumentIds={0}", instrumentIds);
            /*std::vector<std::string> tmpVec = base::split(instrumentIds, ",");
            std::transform(tmpVec.begin(), tmpVec.end(), std::back_inserter(instrumentIdVec_), [](const std::string &s) {
                        char *pc = new char[s.size() + 1];
                        strcpy(pc, s.c_str());
                        return pc;
            });*/

            log_warn("brokerId_={0}, userId_={1}, frontAddr_={2}", broker_id_, user_id_, address_);
        }

        CtpMdApi::~CtpMdApi() {
            log_warn("CtpMdApi deconstructor");
            //delete mdApi_;
            //delete mdSpiCbk_;
            /*for (size_t i = 0; i != instrumentIdArrLen_; ++i) {
                delete []instrumentIdArr_[i];
            }
            delete []instrumentIdArr_;
            */
        }


        void CtpMdApi::close() {
            if (md_api_) {
                md_api_->RegisterSpi(nullptr);
                md_api_->Release();
                md_api_ = nullptr;
            }
        }

        void CtpMdApi::join() {
            md_api_->Join();
        }

        void CtpMdApi::connect() {
            if (!connected_) {

                log_warn("CTP行情接口初始化开始");
                //创建MdApi
                std::string flowPath = "./Response/" + user_id_ + "-MD-";
                md_api_ = CThostFtdcMdApi::CreateFtdcMdApi(flowPath.c_str());
                //回调对象注入接口类
                md_api_->RegisterSpi(this);
                //注册行情前置地址
                md_api_->RegisterFront(const_cast<char *>(address_.c_str()));
                //接口线程启动
                md_api_->Init();
                log_warn("CTP行情接口初始化完毕");
                //mdApi_->Join();
                //sleep(100);
            } else {
                login();
            }
        }

        void CtpMdApi::OnFrontConnected() {
            log_warn("行情服务，与CTP前置连接成功");
            connected_ = true;
            login();
        }

        void CtpMdApi::OnFrontDisconnected(int nReason) {
            log_warn("行情服务,与CTP前置断开连接，原因：{0}", global::mapDisconReasonVal(nReason));
            connected_ = false;
            logged_in_ = false;
            //mdApiWrap_->reqUserLogin();
        }

        void CtpMdApi::login() {
            if (!logged_in_) {
                log_warn("行情服务，发送用户登录请求开始");
                //sleep(20);
                CThostFtdcReqUserLoginField req;
                //memset(&req, 0, sizeof(req));
                strcpy(req.BrokerID, broker_id_.c_str());
                strcpy(req.UserID, user_id_.c_str());
                strcpy(req.Password, password_.c_str());
                int nRequestID = next_req_id();
                int ret = md_api_->ReqUserLogin(&req, nRequestID);
                if (ret == 0) {
                    log_warn("nRequestID={0}，行情服务，发送用户登录请求成功", nRequestID);
                    return;
                }
                log_warn("行情服务，请求用户登录失败，原因：{0}。", global::mapRetVal(ret));
            } else {
                gateway_->subscribe();
            }
        }

        void CtpMdApi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo,
                                      int nRequestID, bool bIsLast) {
            //assert(pRspInfo);
            if (pRspInfo->ErrorID) {
                log_error("nRequestID={0}，行情服务，用户{1}登录失败，原因：{2}", nRequestID, pRspUserLogin->UserID,
                          CONVERT_CTP_STR(pRspInfo->ErrorMsg));
            } else if (bIsLast) {
                log_warn("nRequestID={0}，行情服务，用户{1}登录成功", nRequestID, pRspUserLogin->UserID);
                logged_in_ = true;
                gateway_->notify();
                gateway_->subscribe();
            }
        }

        void CtpMdApi::logout() {
            log_warn("行情服务，发送用户登出请求开始");
            CThostFtdcUserLogoutField req;
            //memset(&req, 0, sizeof(req));
            strcpy(req.BrokerID, broker_id_.c_str());
            strcpy(req.UserID, user_id_.c_str());
            int nRequestID = next_req_id();
            int ret = md_api_->ReqUserLogout(&req, nRequestID);
            if (ret == 0) {
                log_warn("nRequestID={0}，行情服务，发送用户登出请求成功", nRequestID);
                return;
            }
            log_warn("行情服务，请求用户登出失败，原因：{0}。", global::mapRetVal(ret));
        }

        void CtpMdApi::OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo,
                                       int nRequestID, bool bIsLast) {
            //assert(pRspInfo);
            if (pRspInfo->ErrorID) {
                log_error("nRequestID={0}，行情服务，用户{1}登出失败，原因：{2}", nRequestID, pUserLogout->UserID,
                          CONVERT_CTP_STR(pRspInfo->ErrorMsg));
            } else if (bIsLast) {
                log_warn("nRequestID={0}，行情服务，用户{1}登出成功", nRequestID, pUserLogout->UserID);
                logged_in_ = false;
            }
        }

        void CtpMdApi::subscribe(const std::vector<std::string> &subscribed_vec) {
            std::vector<char *> tmp_vec;
            std::transform(subscribed_vec.begin(), subscribed_vec.end(), std::back_inserter(tmp_vec),
                           [](const std::string &s) {
                               char *pc = new char[s.size() + 1];
                               strcpy(pc, s.c_str());
                               return pc;
                           });
            base::raii gard_r([&tmp_vec]() {
                for (auto &pc : tmp_vec) {
                    delete []pc;
                }
            });
            int ret = md_api_->SubscribeMarketData(&tmp_vec[0], tmp_vec.size());
            if (ret == 0) {
                log_info("发送订阅行情数据请求成功");
                return;
            }
            log_warn("行情服务，请求订阅行情数据失败，原因：{0}。", global::mapRetVal(ret));
        }

        void CtpMdApi::OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument,
                                          CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
            //assert(pRspInfo);
            if (pRspInfo->ErrorID) {
                log_error("nRequestID={0}，订阅合约失败，原因：{1}", nRequestID, pRspInfo->ErrorMsg);
            } else {
                log_warn("nRequestID={0}，订阅合约成功，合约号：{1}", nRequestID, pSpecificInstrument->InstrumentID);
            }
        }

        void CtpMdApi::unsubscribe(const std::vector<std::string> &unsubscribed_vec) {
            std::vector<char *> tmp_vec;
            std::transform(unsubscribed_vec.begin(), unsubscribed_vec.end(), std::back_inserter(tmp_vec),
                           [](const std::string &s) {
                               char *pc = new char[s.size() + 1];
                               strcpy(pc, s.c_str());
                               return pc;
                           });
            base::raii gard_r([&]() {
                for (auto &pc : tmp_vec) {
                    delete pc;
                }
            });
            int ret = md_api_->UnSubscribeMarketData(&tmp_vec[0], tmp_vec.size());
            if (ret == 0) {
                log_info("发送取消订阅行情数据请求成功");
                return;
            }
            log_warn("行情服务，请求取消订阅行情数据失败，原因：{0}。", global::mapRetVal(ret));
        }

        void CtpMdApi::OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument,
                                            CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
            //assert(pRspInfo);
            if (pRspInfo->ErrorID) {
                log_error("nRequestID={0}，取消订阅合约失败，原因：{1}", nRequestID, CONVERT_CTP_STR(pRspInfo->ErrorMsg));
            } else {
                log_warn("nRequestID={0}，取消订阅合约成功，合约号：{1}", nRequestID, pSpecificInstrument->InstrumentID);
            }
        }

        using std::chrono::system_clock;
        static const auto one_day = std::chrono::hours(24);
        static std::string now_date = base::strftimep(system_clock::now(), "%Y%m%d");
        static bool switch_flag = false;

        static std::string adjust_action_date(const std::string &src_action_date, const std::string &update_time) {
            std::string dest_action_date;
            if (update_time >= "23:55:00") {
                auto now = system_clock::now();
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
                auto now = system_clock::now();
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

        void CtpMdApi::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData) {
//            log_warn("!!!!!!!!!!!!!!!!trading_date={0}!!!!!!!!!!!!!!!!!!!!!!!!!", Env::instance()->trading_date);
//            exit(0);
            auto pmd = std::make_shared<model::Tick>(pDepthMarketData,
                                                     adjust_action_date(pDepthMarketData->ActionDay,
                                                                        pDepthMarketData->UpdateTime),
                                                     Env::instance()->trading_date);
            gateway_->on_tick(pmd);

/*#ifndef NDEBUG
            //#define DETAIL
            std::ostringstream oss;
            oss << std::endl << "---------------------深度行情---------------------" << std::endl
                << "# 交易日:\t+" << pDepthMarketData->TradingDay << "+" << std::endl
                << "# 合约代码:\t+" << pDepthMarketData->InstrumentID << "+" << std::endl
                #ifdef DETAIL
                << "# 交易所代码:\t+" << pDepthMarketData->ExchangeID << "+" << std::endl
		<< "# 合约在交易所的代码:\t+" << pDepthMarketData->ExchangeInstID << "+" << std::endl
                #endif
                << "# 最新价:\t+" << pDepthMarketData->LastPrice << "+" << std::endl
                << "# 上次结算价:\t+" << pDepthMarketData->PreSettlementPrice << "+" << std::endl
                << "# 昨收盘:\t+" << pDepthMarketData->PreClosePrice << "+" << std::endl
                << "# 昨持仓量:\t+" << pDepthMarketData->PreOpenInterest << "+" << std::endl
                << "# 今开盘:\t+" << pDepthMarketData->OpenPrice << "+" << std::endl
                << "# 最高价:\t+" << pDepthMarketData->HighestPrice << "+" << std::endl
                << "# 最低价:\t+" << pDepthMarketData->LowestPrice << "+" << std::endl
                << "# 数量:\t+" << pDepthMarketData->Volume << "+" << std::endl
                << "# 成交金额:\t+" << pDepthMarketData->Turnover << "+" << std::endl
                << "# 持仓量:\t+" << pDepthMarketData->OpenInterest << "+" << std::endl
                #ifdef DETAIL
                << "# 今收盘:\t+" << pDepthMarketData->ClosePrice << "+" << std::endl
		<< "# 本次结算价:\t+" << pDepthMarketData->SettlementPrice << "+" << std::endl
                #endif
                << "# 涨停板价:\t+" << pDepthMarketData->UpperLimitPrice << "+" << std::endl
                << "# 跌停板价:\t+" << pDepthMarketData->LowerLimitPrice << "+" << std::endl
                #ifdef DETAIL
                << "# 昨虚实度:\t+" << pDepthMarketData->PreDelta << "+" << std::endl
		<< "# 今虚实度:\t+" << pDepthMarketData->CurrDelta << "+" << std::endl
                #endif
                << "# 最后修改时间:\t+" << pDepthMarketData->UpdateTime << "+" << std::endl
                << "# 最后修改毫秒:\t+" << pDepthMarketData->UpdateMillisec << "+" << std::endl
                << "# 申买价一:\t+" << pDepthMarketData->BidPrice1 << "+" << std::endl
                << "# 申买量一:\t+" << pDepthMarketData->BidVolume1 << "+" << std::endl
                << "# 申卖价一:\t+" << pDepthMarketData->AskPrice1 << "+" << std::endl
                << "# 申卖量一:\t+" << pDepthMarketData->AskVolume1 << "+" << std::endl
                #ifdef DETAIL
                << "# 申买价二:\t+" << pDepthMarketData->BidPrice2 << "+" << std::endl
		<< "# 申买量二:\t+" << pDepthMarketData->BidVolume2 << "+" << std::endl
		<< "# 申卖价二:\t+" << pDepthMarketData->AskPrice2 << "+" << std::endl
		<< "# 申卖量二:\t+" << pDepthMarketData->AskVolume2 << "+" << std::endl
		<< "# 申买价三:\t+" << pDepthMarketData->BidPrice3 << "+" << std::endl
		<< "# 申买量三:\t+" << pDepthMarketData->BidVolume3 << "+" << std::endl
		<< "# 申卖价三:\t+" << pDepthMarketData->AskPrice3 << "+" << std::endl
		<< "# 申卖量三:\t+" << pDepthMarketData->AskVolume3 << "+" << std::endl
		<< "# 申买价四:\t+" << pDepthMarketData->BidPrice4 << "+" << std::endl
		<< "# 申买量四:\t+" << pDepthMarketData->BidVolume4 << "+" << std::endl
		<< "# 申卖价四:\t+" << pDepthMarketData->AskPrice4 << "+" << std::endl
		<< "# 申卖量四:\t+" << pDepthMarketData->AskVolume4 << "+" << std::endl
		<< "# 申买价五:\t+" << pDepthMarketData->BidPrice5 << "+" << std::endl
		<< "# 申买量五:\t+" << pDepthMarketData->BidVolume5 << "+" << std::endl
		<< "# 申卖价五:\t+" << pDepthMarketData->AskPrice5 << "+" << std::endl
		<< "# 申卖量五:\t+" << pDepthMarketData->AskVolume5 << "+" << std::endl
		<< "# 当日均价:\t+" << pDepthMarketData->AveragePrice << "+" << std::endl
		<< "# 业务日期:\t+" << pDepthMarketData->ActionDay << "+"
#undef DETAIL
                #endif
                << std::endl;
             log_trace(oss.str());
#endif*/
        }

        void CtpMdApi::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
            //assert(pRspInfo);
            if (pRspInfo->ErrorID) {
                log_error("nRequestID={0}，行情服务，CTP前置返回错误，原因：{1}", nRequestID, CONVERT_CTP_STR(pRspInfo->ErrorMsg));
            }
        }


    } /* ctp */

} /* iqt */ 

