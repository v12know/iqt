#include "ctp/trade_api.h"

#include <unistd.h>
#include <base/exception.h>
#include <base/enum_wrap.hpp>
#include <env.h>

#include "trade/qry_queue.h"
#include "base/log.h"
#include "global.h"
#include "model/order.h"
#include "event/event.h"
#include "ctp/trade_gateway.h"

#include "model/instrument.h"
#include "model/order.h"
#include "model/position.h"
#include "model/account.h"
#include "model/future_info.h"

#include "base/enum_wrap.hpp"

namespace iqt {
    namespace trade {
/*        static const char *skResumeTypeArr[]{"restart", "resume", "quick"};
        static constexpr size_t skResumeTypeArrLen = sizeof(skResumeTypeArr) / sizeof(skResumeTypeArr[0]);

        static inline THOST_TE_RESUME_TYPE toResumeTypeEnum(const std::string &resumeType) {
            for (size_t i = 0; i != skResumeTypeArrLen; ++i) {
                if (resumeType.compare(skResumeTypeArr[i]) == 0) {
                    return static_cast<THOST_TE_RESUME_TYPE>(i);
                }
            }
            log_warn("Can't match any resume type, will use THOST_TERT_RESUME");
            return THOST_TERT_RESUME;
        }*/

        static base::EnumWrap<THOST_TE_RESUME_TYPE, std::string> s_resume_type({"restart", "resume", "quick"});

        CtpTradeApi::CtpTradeApi(CtpTradeGateway *gateway, const std::string &broker_id, const std::string &user_id, const std::string &password,
                             const std::string &address, const std::string &pub_resume_type,
                             const std::string &priv_resume_type) :
                gateway_(gateway), qry_queue_(new trade::QryQueue()),
                broker_id_(broker_id), user_id_(user_id), password_(password), address_(address),
                pub_resume_type_(pub_resume_type), priv_resume_type_(priv_resume_type) {
            log_warn("broker_id_={0}, user_id_={1}, address_={2}, pub_resume_type_={3}, priv_resume_type_={4}",
                     broker_id_, user_id_, address_, pub_resume_type_, priv_resume_type_);
            qry_queue_->start();
        }

        CtpTradeApi::~CtpTradeApi() {
            log_warn("TdApiWrap deconstructor start");
            //delete tdApi_;
            //delete tdSpiCbk_;
            delete qry_queue_;
            // delete tdPreload_;
            log_warn("TdApiWrap deconstructor end");
        }

        void CtpTradeApi::close() {
            if (td_api_) {
                td_api_->RegisterSpi(nullptr);
                td_api_->Release();
                td_api_ = nullptr;
            }
        }


        void CtpTradeApi::join() {
            td_api_->Join();
        }

        const char *CtpTradeApi::getTradingDay() {
            return td_api_->GetTradingDay();
        }

        void CtpTradeApi::connect() {
            if (!connected_) {
                log_warn("CTP交易接口初始化开始");
                //初始化TraderApi
                std::string flowPath = "./Response/" + user_id_ + "-TD-";
                td_api_ = CThostFtdcTraderApi::CreateFtdcTraderApi(flowPath.c_str());

                //回调对象注入接口类
                td_api_->RegisterSpi(this);
                //注册公有流
//                td_api_->SubscribePublicTopic(toResumeTypeEnum(pub_resume_type_));
                td_api_->SubscribePublicTopic(s_resume_type.literal2enum(pub_resume_type_));
                //注册私有流
//                td_api_->SubscribePrivateTopic(toResumeTypeEnum(priv_resume_type_));
                td_api_->SubscribePrivateTopic(s_resume_type.literal2enum(priv_resume_type_));
                //注册行情前置地址
                td_api_->RegisterFront(const_cast<char *>(address_.c_str()));
                //接口线程启动
                td_api_->Init();
                log_warn("CTP交易接口初始化完毕");
            } else {
                if (authenticated_) {
                    authenticate();
                } else {
                    login();
                }
            }
        }

///当客户端与交易后台建立起通信连接时（还未登录前），该方法被调用。
        void CtpTradeApi::OnFrontConnected() {
            log_warn("交易服务，与CTP前置连接成功");
            connected_ = true;
            if (require_authentication_) {
                authenticate();
            } else {
                login();
            }
        }


///当客户端与交易后台通信连接断开时，该方法被调用。当发生这个情况后，API会自动重新连接，客户端可不做处理。
///@param nReason 错误原因
///        0x1001 网络读失败
///        0x1002 网络写失败
///        0x2001 接收心跳超时
///        0x2002 发送心跳失败
///        0x2003 收到错误报文
        void CtpTradeApi::OnFrontDisconnected(int nReason) {
            log_warn("交易服务，与CTP前置断开连接，原因：{0}", global::mapDisconReasonVal(nReason));
            connected_ = false;
            logged_in_ = false;
            log_warn("服务器断开，将自动重连。");
        }

        void CtpTradeApi::login() {
            if (!logged_in_) {
                log_warn("交易服务，请求用户登录开始");
                CThostFtdcReqUserLoginField req;
                //memset(&req, 0, sizeof(req));
                strcpy(req.BrokerID, broker_id_.c_str());
                strcpy(req.UserID, user_id_.c_str());
                strcpy(req.Password, password_.c_str());

                int nRequestID = qry_queue_->next_req_id();
                int ret = td_api_->ReqUserLogin(&req, nRequestID);
                if (ret == 0) {
                    log_warn("nRequestID={0}，交易服务，请求用户登录成功", nRequestID);
                    return;
                }
                log_warn("交易服务，请求用户登录失败，原因：{0}。", global::mapRetVal(ret));
            }
        }

        ///登录请求响应
        void
        CtpTradeApi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo,
                                  int nRequestID, bool bIsLast) {
            assert(pRspUserLogin);
            if (pRspInfo && pRspInfo->ErrorID) {
                log_warn("nRequestID={0}，交易服务，用户{1}登录失败，原因：[{2}]{3}", nRequestID, pRspUserLogin->UserID, pRspInfo->ErrorID, CONVERT_CTP_STR(pRspInfo->ErrorMsg));
                exit(1);
            }

            logged_in_ = true;
            log_warn("nRequestID={0}，交易服务，用户{1}登录成功", nRequestID, pRspUserLogin->UserID);
            set_fs_id(pRspUserLogin->FrontID, pRspUserLogin->SessionID);
            set_order_ref(pRspUserLogin->MaxOrderRef);
            log_warn(
                    "FrontID={0}, SessionID={1}, MaxOrderRef={2}, SHFETime={3}, DCETime={4}, CZCETime={5}, FFEXTime={6}, INETime={7}",
                    pRspUserLogin->FrontID, pRspUserLogin->SessionID, pRspUserLogin->MaxOrderRef,
                    pRspUserLogin->SHFETime, pRspUserLogin->DCETime, pRspUserLogin->CZCETime, pRspUserLogin->FFEXTime,
                    pRspUserLogin->INETime);

            // log_warn("tradingDay={0}", tdApiWrap_->getTradingDay());
            // tdApiWrap_->getTdPreload()->onRspUserLogin();
//            reqSettlementInfoConfirm();
            gateway_->notify();
        }


        void CtpTradeApi::logout() {
            log_warn("交易服务，请求用户登出开始");
            CThostFtdcUserLogoutField req;
            //memset(&req, 0, sizeof(req));
            strcpy(req.BrokerID, broker_id_.c_str());
            strcpy(req.UserID, user_id_.c_str());
            int nRequestID = qry_queue_->next_req_id();
            int ret = td_api_->ReqUserLogout(&req, nRequestID);
            if (ret == 0) {
                log_warn("nRequestID={0}，交易服务，请求用户登出成功", nRequestID);
                return;
            }
            log_warn("交易服务，请求用户登出失败，原因：{0}。", global::mapRetVal(ret));
        }

        ///登出请求响应
        void CtpTradeApi::OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo,
                                              int nRequestID, bool bIsLast) {
            if (pRspInfo && pRspInfo->ErrorID) {
                log_warn("nRequestID={0}，交易服务，用户{1}登出失败，原因：[{2}]{3}", nRequestID, pUserLogout->UserID, pRspInfo->ErrorID, CONVERT_CTP_STR(pRspInfo->ErrorMsg));
                return;
            }
            assert(pUserLogout);
            logged_in_ = false;
            log_warn("nRequestID={0}，交易服务，用户{1}登出成功", nRequestID, pUserLogout->UserID);
        }

        void CtpTradeApi::authenticate() {
            //TODO
            if (!authenticated_) {
                CThostFtdcReqAuthenticateField req;
                memset(&req, 0, sizeof(req));
                strcpy(req.BrokerID, broker_id_.c_str());
                strcpy(req.UserID, user_id_.c_str());
                int nRequestID = qry_queue_->next_req_id();
                int ret = td_api_->ReqAuthenticate(&req, nRequestID);
                if (ret == 0) {
                    log_warn("nRequestID={0}，交易服务，请求申请认证成功", nRequestID);
                    return;
                }
                log_warn("交易服务，请求申请认证失败，原因：{0}。", global::mapRetVal(ret));
            } else {
                login();
            }
        }

        void CtpTradeApi::OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {

            if (pRspInfo && pRspInfo->ErrorID) {
                log_warn("nRequestID={0}，认证失败，原因：[{1}]{2}", nRequestID, pRspInfo->ErrorID, CONVERT_CTP_STR(pRspInfo->ErrorMsg));
                return;
            }
            authenticated_ = true;
            login();
        }



        model::OrderMapPtr CtpTradeApi::qry_order() {
            auto lambda = [this](int nRequestID) {
                order_cache_ = nullptr;
                log_warn("请求查询下单开始");
                CThostFtdcQryOrderField req;
                //memset(&req, 0, sizeof(req));
                strcpy(req.BrokerID, broker_id_.c_str());
                strcpy(req.InvestorID, user_id_.c_str());
                int ret = td_api_->ReqQryOrder(&req, nRequestID);
                if (ret == 0) {
                    log_debug("nRequestID={0}，请求查询下单成功", nRequestID);
                } else {
                    log_warn("请求查询下单失败，原因：{0}", global::mapRetVal(ret));
                }
                return ret;
            };
            return qry_queue_->qry<model::OrderMap>(lambda);
        }
        ///请求查询报单响应
        void
        CtpTradeApi::OnRspQryOrder(CThostFtdcOrderField *pOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                                 bool bIsLast) {

            if (pRspInfo && pRspInfo->ErrorID) {
                auto msg = CONVERT_CTP_STR(pRspInfo->ErrorMsg);
                log_warn("nRequestID={0}，请求查询报单失败，原因：[{1}]{2}", nRequestID, pRspInfo->ErrorID, msg);
                qry_queue_->on_err(nRequestID, pRspInfo->ErrorID, msg);
                return;
            }
            if (!order_cache_) {
                order_cache_ = std::make_shared<model::OrderMap>();
            }
            if (!pOrder) {
                log_warn("nRequestID={0}，请求查询报单pOrder为空", nRequestID);
                qry_queue_->on_rsp(nRequestID, order_cache_);
                return;
            }
            auto order = std::make_shared<model::Order>(pOrder);
            (*order_cache_)[order->order_id] = order;
            if (bIsLast) {
                qry_queue_->on_rsp(nRequestID, order_cache_);
                order_cache_ = nullptr;
            }
            std::ostringstream oss;
            oss << std::endl << "---------------------nRequestID=" << nRequestID << "，报单---------------------"
                << std::endl;
        }

        void CtpTradeApi::reqQryTrade() {
            auto lambda = [this](int nRequestID) {
                log_warn("请求查询成交情况开始");
                CThostFtdcQryTradeField req;
                //memset(&req, 0, sizeof(req));
                strcpy(req.BrokerID, broker_id_.c_str());
                strcpy(req.InvestorID, user_id_.c_str());
                int ret = td_api_->ReqQryTrade(&req, nRequestID);
                if (ret == 0) {
                    log_debug("nRequestID={0}，请求查询成交情况成功", nRequestID);
                } else {
                    log_warn("请求查询成交情况失败，原因：{0}", global::mapRetVal(ret));
                }
                return ret;
            };
            qry_queue_->qry<void>(lambda);
        }
        ///请求查询成交响应
        void
        CtpTradeApi::OnRspQryTrade(CThostFtdcTradeField *pTrade, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                                 bool bIsLast) {}

        model::PositionMapPtr CtpTradeApi::qry_position() {
            auto lambda = [this](int nRequestID) {
                pos_cache_ = nullptr;
                log_warn("请求查询投资者持仓开始");
                CThostFtdcQryInvestorPositionField req;
                memset(&req, 0, sizeof(req));
                strcpy(req.BrokerID, broker_id_.c_str());
                strcpy(req.InvestorID, user_id_.c_str());
                int ret = td_api_->ReqQryInvestorPosition(&req, nRequestID);
                if (ret == 0) {
                    log_debug("nRequestID={0}，请求查询投资者持仓成功", nRequestID);
                } else {
                    log_warn("请求查询投资者持仓失败，原因：{0}", global::mapRetVal(ret));
                }
                return ret;
            };
            return qry_queue_->qry<model::PositionMap>(lambda);

        }

        ///请求查询投资者持仓响应
        void CtpTradeApi::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition,
                                                        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
            if (pRspInfo && pRspInfo->ErrorID) {
                auto msg = CONVERT_CTP_STR(pRspInfo->ErrorMsg);
                log_warn("nRequestID={0}，请求查询投资者持仓失败，原因：{1}", nRequestID, msg);
                qry_queue_->on_err(nRequestID, pRspInfo->ErrorID, msg);
                return;
            }
            if (!pos_cache_) {
                pos_cache_ = std::make_shared<model::PositionMap>();
            }
            if (!pInvestorPosition) {
                log_warn("nRequestID={0}，请求查询投资者持仓pInvestorPosition为空");
                qry_queue_->on_rsp(nRequestID, pos_cache_);
                return;
            }
            if (pInvestorPosition->InstrumentID) {
                auto order_book_id = global::make_order_book_id(pInvestorPosition->InstrumentID);
                auto iter = pos_cache_->find(order_book_id);
                if (iter == pos_cache_->end()) {
                    auto pos = std::make_shared<model::Position>(pInvestorPosition);
                    (*pos_cache_)[order_book_id] = pos;
                } else {
                    (*pos_cache_)[order_book_id]->update(pInvestorPosition);
                }
            }
            if (bIsLast) {
                qry_queue_->on_rsp(nRequestID, pos_cache_);
                pos_cache_ = nullptr;
            }
            std::ostringstream oss;
            oss << std::endl << "---------------------nRequestID=" << nRequestID << "，投资者持仓---------------------"
                << std::endl
                << "# 合约代码:\t+" << pInvestorPosition->InstrumentID << "+" << std::endl
                << "# 经纪公司代码:\t+" << pInvestorPosition->BrokerID << "+" << std::endl
                << "# 投资者代码:\t+" << pInvestorPosition->InvestorID << "+" << std::endl
                << "# 持仓多空方向:\t+" << pInvestorPosition->PosiDirection << "+" << std::endl
                << "# 投机套保标志:\t+" << pInvestorPosition->HedgeFlag << "+" << std::endl
                << "# 持仓日期:\t+" << pInvestorPosition->PositionDate << "+" << std::endl
                << "# 上日持仓(收盘时):\t+" << pInvestorPosition->YdPosition << "+" << std::endl
                << "# 当前持仓:\t+" << pInvestorPosition->Position << "+" << std::endl
                << "# 多头冻结:\t+" << pInvestorPosition->LongFrozen << "+" << std::endl
                << "# 空头冻结:\t+" << pInvestorPosition->ShortFrozen << "+" << std::endl
                << "# 开多仓冻结金额:\t+" << pInvestorPosition->LongFrozenAmount << "+" << std::endl
                << "# 开空仓冻结金额:\t+" << pInvestorPosition->ShortFrozenAmount << "+" << std::endl
                << "# 开仓量:\t+" << pInvestorPosition->OpenVolume << "+" << std::endl
                << "# 平仓量:\t+" << pInvestorPosition->CloseVolume << "+" << std::endl
                << "# 开仓金额:\t+" << pInvestorPosition->OpenAmount << "+" << std::endl
                << "# 平仓金额:\t+" << pInvestorPosition->CloseAmount << "+" << std::endl
                << "# 持仓成本:\t+" << pInvestorPosition->PositionCost << "+" << std::endl
                << "# 上次占用的保证金:\t+" << pInvestorPosition->PreMargin << "+" << std::endl
                << "# 占用的保证金:\t+" << pInvestorPosition->UseMargin << "+" << std::endl
                << "# 冻结的保证金:\t+" << pInvestorPosition->FrozenMargin << "+" << std::endl
                << "# 冻结的资金:\t+" << pInvestorPosition->FrozenCash << "+" << std::endl
                << "# 冻结的手续费:\t+" << pInvestorPosition->FrozenCommission << "+" << std::endl
                << "# 资金差额:\t+" << pInvestorPosition->CashIn << "+" << std::endl
                << "# 手续费:\t+" << pInvestorPosition->Commission << "+" << std::endl
                << "# 平仓盈亏:\t+" << pInvestorPosition->CloseProfit << "+" << std::endl
                << "# 持仓盈亏:\t+" << pInvestorPosition->PositionProfit << "+" << std::endl
                << "# 上次结算价:\t+" << pInvestorPosition->PreSettlementPrice << "+" << std::endl
                << "# 本次结算价:\t+" << pInvestorPosition->SettlementPrice << "+" << std::endl
                << "# 交易日:\t+" << pInvestorPosition->TradingDay << "+" << std::endl
                << "# 结算编号:\t+" << pInvestorPosition->SettlementID << "+" << std::endl
                << "# 开仓成本:\t+" << pInvestorPosition->OpenCost << "+" << std::endl
                << "# 交易所保证金:\t+" << pInvestorPosition->ExchangeMargin << "+" << std::endl
                << "# 组合成交形成的持仓:\t+" << pInvestorPosition->CombPosition << "+" << std::endl
                << "# 组合多头冻结:\t+" << pInvestorPosition->CombLongFrozen << "+" << std::endl
                << "# 组合空头冻结:\t+" << pInvestorPosition->CombShortFrozen << "+" << std::endl
                << "# 逐日盯市平仓盈亏:\t+" << pInvestorPosition->CloseProfitByDate << "+" << std::endl
                << "# 逐笔对冲平仓盈亏:\t+" << pInvestorPosition->CloseProfitByTrade << "+" << std::endl
                << "# 今日持仓:\t+" << pInvestorPosition->TodayPosition << "+" << std::endl
                << "# 保证金率:\t+" << pInvestorPosition->MarginRateByMoney << "+" << std::endl
                << "# 保证金率(按手数):\t+" << pInvestorPosition->MarginRateByVolume << "+" << std::endl
                << "# 执行冻结:\t+" << pInvestorPosition->StrikeFrozen << "+" << std::endl
                << "# 执行冻结金额:\t+" << pInvestorPosition->StrikeFrozenAmount << "+" << std::endl
                << "# 放弃执行冻结:\t+" << pInvestorPosition->AbandonFrozen << "+" << std::endl;
            log_warn(oss.str());
        }

        std::shared_ptr<model::Account> CtpTradeApi::qry_account() {
            auto lambda = [this](int nRequestID) {
                log_warn("请求查询资金账户开始");
                CThostFtdcQryTradingAccountField req;
                memset(&req, 0, sizeof(req));
                strcpy(req.BrokerID, broker_id_.c_str());
                strcpy(req.InvestorID, user_id_.c_str());
                int ret = td_api_->ReqQryTradingAccount(&req, nRequestID);
                if (ret == 0) {
                    log_debug("nRequestID={0}，请求查询资金账户成功", nRequestID);
                } else {
                    log_warn("请求查询资金账户失败，原因：{0}", global::mapRetVal(ret));
                }
                return ret;
            };
            return qry_queue_->qry<model::Account>(lambda);
        }

        ///请求查询资金账户响应
        void CtpTradeApi::OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount,
                                                      CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
            if (pRspInfo && pRspInfo->ErrorID) {
                auto msg = CONVERT_CTP_STR(pRspInfo->ErrorMsg);
                log_warn("nRequestID={0}，请求查询资金账户失败，原因：{1}", nRequestID, msg);
                qry_queue_->on_err(nRequestID, pRspInfo->ErrorID, msg);
                return;
            }
            std::shared_ptr<model::Account> account = nullptr;
            if (!pTradingAccount) {
                log_warn("nRequestID={0}，请求查询资金账户pTradingAccount为空");
                qry_queue_->on_rsp(nRequestID, account);
                return;
            }
            if (bIsLast) {
                account = std::make_shared<model::Account>(pTradingAccount);
                qry_queue_->on_rsp(nRequestID, account);
            }

            std::ostringstream oss;
            oss << std::endl << "---------------------nRequestID=" << nRequestID << "，资金账户---------------------"
                << std::endl
                << "# 经纪公司代码:\t+" << pTradingAccount->BrokerID << "+" << std::endl
                << "# 投资者帐号:\t+" << pTradingAccount->AccountID << "+" << std::endl
                << "# 上次质押金额:\t+" << pTradingAccount->PreMortgage << "+" << std::endl
                << "# 上次信用额度:\t+" << pTradingAccount->PreCredit << "+" << std::endl
                << "# 上次存款额:\t+" << pTradingAccount->PreDeposit << "+" << std::endl
                << "# 上次结算准备金:\t+" << pTradingAccount->PreBalance << "+" << std::endl
                << "# 上次占用的保证金:\t+" << pTradingAccount->PreMargin << "+" << std::endl
                << "# 利息基数:\t+" << pTradingAccount->InterestBase << "+" << std::endl
                << "# 利息收入:\t+" << pTradingAccount->Interest << "+" << std::endl
                << "# 入金金额:\t+" << pTradingAccount->Deposit << "+" << std::endl
                << "# 出金金额:\t+" << pTradingAccount->Withdraw << "+" << std::endl
                << "# 冻结的保证金:\t+" << pTradingAccount->FrozenMargin << "+" << std::endl
                << "# 冻结的资金:\t+" << pTradingAccount->FrozenCash << "+" << std::endl
                << "# 冻结的手续费:\t+" << pTradingAccount->FrozenCommission << "+" << std::endl
                << "# 当前保证金总额:\t+" << pTradingAccount->CurrMargin << "+" << std::endl
                << "# 资金差额:\t+" << pTradingAccount->CashIn << "+" << std::endl
                << "# 手续费:\t+" << pTradingAccount->Commission << "+" << std::endl
                << "# 平仓盈亏:\t+" << pTradingAccount->CloseProfit << "+" << std::endl
                << "# 持仓盈亏:\t+" << pTradingAccount->PositionProfit << "+" << std::endl
                << "# 期货结算准备金:\t+" << pTradingAccount->Balance << "+" << std::endl
                << "# 可用资金:\t+" << pTradingAccount->Available << "+" << std::endl
                << "# 可取资金:\t+" << pTradingAccount->WithdrawQuota << "+" << std::endl
                << "# 基本准备金:\t+" << pTradingAccount->Reserve << "+" << std::endl
                << "# 交易日:\t+" << pTradingAccount->TradingDay << "+" << std::endl
                << "# 结算编号:\t+" << pTradingAccount->SettlementID << "+" << std::endl
                << "# 信用额度:\t+" << pTradingAccount->Credit << "+" << std::endl
                << "# 质押金额:\t+" << pTradingAccount->Mortgage << "+" << std::endl
                << "# 交易所保证金:\t+" << pTradingAccount->ExchangeMargin << "+" << std::endl
                << "# 投资者交割保证金:\t+" << pTradingAccount->DeliveryMargin << "+" << std::endl
                << "# 交易所交割保证金:\t+" << pTradingAccount->ExchangeDeliveryMargin << "+" << std::endl
                << "# 保底期货结算准备金:\t+" << pTradingAccount->ReserveBalance << "+" << std::endl
                << "# 币种代码:\t+" << pTradingAccount->CurrencyID << "+" << std::endl
                << "# 上次货币质入金额:\t+" << pTradingAccount->PreFundMortgageIn << "+" << std::endl
                << "# 上次货币质出金额:\t+" << pTradingAccount->PreFundMortgageOut << "+" << std::endl
                << "# 货币质入金额:\t+" << pTradingAccount->FundMortgageIn << "+" << std::endl
                << "# 货币质出金额:\t+" << pTradingAccount->FundMortgageOut << "+" << std::endl
                << "# 货币质押余额:\t+" << pTradingAccount->FundMortgageAvailable << "+" << std::endl
                << "# 可质押货币金额:\t+" << pTradingAccount->MortgageableFund << "+" << std::endl
                << "# 特殊产品占用保证金:\t+" << pTradingAccount->SpecProductMargin << "+" << std::endl
                << "# 特殊产品冻结保证金:\t+" << pTradingAccount->SpecProductFrozenMargin << "+" << std::endl
                << "# 特殊产品手续费:\t+" << pTradingAccount->SpecProductCommission << "+" << std::endl
                << "# 特殊产品冻结手续费:\t+" << pTradingAccount->SpecProductFrozenCommission << "+" << std::endl
                << "# 特殊产品持仓盈亏:\t+" << pTradingAccount->SpecProductPositionProfit << "+" << std::endl
                << "# 特殊产品平仓盈亏:\t+" << pTradingAccount->SpecProductCloseProfit << "+" << std::endl
                << "# 根据持仓盈亏算法计算的特殊产品持仓盈亏:\t+" << pTradingAccount->SpecProductPositionProfitByAlg << "+" << std::endl
                << "# 特殊产品交易所保证金:\t+" << pTradingAccount->SpecProductExchangeMargin << "+" << std::endl;
            log_warn(oss.str());
        }

        void CtpTradeApi::reqQryInvestor() {
            auto lambda = [this](int nRequestID) {
                log_warn("请求查询资金账户开始");
                CThostFtdcQryInvestorField req;
                //memset(&req, 0, sizeof(req));
                strcpy(req.BrokerID, broker_id_.c_str());
                strcpy(req.InvestorID, user_id_.c_str());
                int ret = td_api_->ReqQryInvestor(&req, nRequestID);
                if (ret == 0) {
                    log_debug("nRequestID={0}，请求查询资金账户成功", nRequestID);
                } else {
                    log_warn("请求查询资金账户失败，原因：{0}", global::mapRetVal(ret));
                }
                return ret;
            };
            qry_queue_->qry<void>(lambda);
        }
        ///请求查询投资者响应
        void
        CtpTradeApi::OnRspQryInvestor(CThostFtdcInvestorField *pInvestor, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                                    bool bIsLast) {}

        void CtpTradeApi::reqQryTradingCode() {
            auto lambda = [this](int nRequestID) {
                log_warn("请求查询交易编码开始");
                CThostFtdcQryTradingCodeField req;
                //memset(&req, 0, sizeof(req));
                strcpy(req.BrokerID, broker_id_.c_str());
                strcpy(req.InvestorID, user_id_.c_str());
                int ret = td_api_->ReqQryTradingCode(&req, nRequestID);
                if (ret == 0) {
                    log_debug("nRequestID={0}，请求查询交易编码成功", nRequestID);
                } else {
                    log_warn("请求查询交易编码失败，原因：{0}", global::mapRetVal(ret));
                }
                return ret;
            };
            qry_queue_->qry<void>(lambda);
        }
        ///请求查询交易编码响应
        void
        CtpTradeApi::OnRspQryTradingCode(CThostFtdcTradingCodeField *pTradingCode, CThostFtdcRspInfoField *pRspInfo,
                                       int nRequestID, bool bIsLast) {}

        void CtpTradeApi::reqQryExchange(const std::string &exchangeId) {
            auto lambda = [this, exchangeId](int nRequestID) {
                log_warn("查询交易所开始");
                CThostFtdcQryExchangeField req;
                //memset(&req, 0, sizeof(req));
                strcpy(req.ExchangeID, exchangeId.c_str());
                int ret = td_api_->ReqQryExchange(&req, nRequestID);
                if (ret == 0) {
                    log_debug("nRequestID={0}，查询交易所成功", nRequestID);
                } else {
                    log_warn("查询交易所失败，原因：{0}", global::mapRetVal(ret));
                }
                return ret;
            };
            qry_queue_->qry<void>(lambda);

        }

        ///请求查询交易所响应
        void
        CtpTradeApi::OnRspQryExchange(CThostFtdcExchangeField *pExchange, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                                    bool bIsLast) {}

        model::InstrumentMapPtr CtpTradeApi::qry_instrument(const std::string &instrument_id) {
            auto lambda = [this, instrument_id](int nRequestID) {
                ins_cache_ = nullptr;
                log_warn("请求查询合约开始, instrumentId={0}", instrument_id);
                CThostFtdcQryInstrumentField req;
                memset(&req, 0, sizeof(req));
                //strcpy(req.ExchangeID, exchangeId.c_str());
                if (instrument_id != "") {
                    strcpy(req.InstrumentID, instrument_id.c_str());
                }
                int ret = td_api_->ReqQryInstrument(&req, nRequestID);
                if (ret == 0) {
                    log_debug("nRequestID={0}，请求查询合约成功", nRequestID);
                } else {
                    log_warn("请求查询合约失败，原因：{0}", global::mapRetVal(ret));
                }
                return ret;
            };
            return qry_queue_->qry<model::InstrumentMap>(lambda);
        }

        ///请求查询合约响应
        void
        CtpTradeApi::OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo,
                                      int nRequestID, bool bIsLast) {
            if (pRspInfo && pRspInfo->ErrorID) {
                auto msg = CONVERT_CTP_STR(pRspInfo->ErrorMsg);
                log_warn("nRequestID={0}，查询投资者结算单失败，原因：{1}", nRequestID, msg);
                qry_queue_->on_err(nRequestID, pRspInfo->ErrorID, msg);
                return;
            }
            if (!pInstrument) {
                qry_queue_->on_rsp(nRequestID, ins_cache_);
                log_warn("nRequestID={0}，查询投资者结算单pInstrument为空");
                return;
            }
            if (pInstrument->IsTrading) {
                if (!ins_cache_) {
                    ins_cache_ = std::make_shared<model::InstrumentMap>();
                }
                auto ins = std::make_shared<model::Instrument>(pInstrument);
                (*ins_cache_)[ins->order_book_id] = ins;
            }
            if (bIsLast) {
                qry_queue_->on_rsp(nRequestID, ins_cache_);
                ins_cache_ = nullptr;
            }
            //std::cout << "OptionsType='" << pInstrument->OptionsType << "'" << std::endl;
            //log_debug("OptionsType='{0}'", pInstrument->OptionsType);
            std::ostringstream oss;
            oss << std::endl << "---------------------nRequestID=" << nRequestID << "，合约信息---------------------"
                << std::endl
                << "# 合约代码:\t+" << pInstrument->InstrumentID << "+" << std::endl
                << "# 交易所代码:\t+" << pInstrument->ExchangeID << "+" << std::endl
                << "# 合约名称:\t+" << CONVERT_CTP_STR(pInstrument->InstrumentName) << "+" << std::endl
                << "# 合约在交易所的代码:\t+" << pInstrument->ExchangeInstID << "+" << std::endl
                << "# 产品代码:\t+" << pInstrument->ProductID << "+" << std::endl
                << "# 产品类型:\t+" << pInstrument->ProductClass << "+" << std::endl
                << "# 交割年份:\t+" << pInstrument->DeliveryYear << "+" << std::endl
                << "# 交割月:\t+" << pInstrument->DeliveryMonth << "+" << std::endl
                << "# 市价单最大下单量:\t+" << pInstrument->MaxMarketOrderVolume << "+" << std::endl
                << "# 市价单最小下单量:\t+" << pInstrument->MinMarketOrderVolume << "+" << std::endl
                << "# 限价单最大下单量:\t+" << pInstrument->MaxLimitOrderVolume << "+" << std::endl
                << "# 限价单最小下单量:\t+" << pInstrument->MinLimitOrderVolume << "+" << std::endl
                << "# 合约数量乘数:\t+" << pInstrument->VolumeMultiple << "+" << std::endl
                << "# 最小变动价位:\t+" << pInstrument->PriceTick << "+" << std::endl
                << "# 创建日:\t+" << pInstrument->CreateDate << "+" << std::endl
                << "# 上市日:\t+" << pInstrument->OpenDate << "+" << std::endl
                << "# 到期日:\t+" << pInstrument->ExpireDate << "+" << std::endl
                << "# 开始交割日:\t+" << pInstrument->StartDelivDate << "+" << std::endl
                << "# 结束交割日:\t+" << pInstrument->EndDelivDate << "+" << std::endl
                << "# 合约生命周期状态:\t+" << pInstrument->InstLifePhase << "+" << std::endl
                << "# 当前是否交易:\t+" << pInstrument->IsTrading << "+" << std::endl
                << "# 持仓类型:\t+" << pInstrument->PositionType << "+" << std::endl
                << "# 持仓日期类型:\t+" << pInstrument->PositionDateType << "+" << std::endl
                << "# 多头保证金率:\t+" << pInstrument->LongMarginRatio << "+" << std::endl
                << "# 空头保证金率:\t+" << pInstrument->ShortMarginRatio << "+" << std::endl
                << "# 是否使用大额单边保证金算法:\t+" << pInstrument->MaxMarginSideAlgorithm << "+" << std::endl
                << "# 基础商品代码:\t+" << pInstrument->UnderlyingInstrID << "+" << std::endl
                << "# 执行价:\t+" << pInstrument->StrikePrice << "+" << std::endl
                << "# 期权类型:\t+{0}+" << std::endl
                << "# 合约基础商品乘数:\t+" << pInstrument->UnderlyingMultiple << "+" << std::endl
                << "# 组合类型:\t+" << pInstrument->CombinationType << "+" << std::endl;
//            if (pInstrument->CombinationType != '0')
//            log_warn(oss.str(), pInstrument->OptionsType);

        }

        void CtpTradeApi::reqQrySettlementInfo() {
            auto lambda = [this](int nRequestID) {
                log_warn("请求查询结算单开始");
                CThostFtdcQrySettlementInfoField req;
                //memset(&req, 0, sizeof(req));
                strcpy(req.BrokerID, broker_id_.c_str());
                strcpy(req.InvestorID, user_id_.c_str());
                int ret = td_api_->ReqQrySettlementInfo(&req, nRequestID);
                if (ret == 0) {
                    log_debug("nRequestID={0}，请求查询结算单成功", nRequestID);
                } else {
                    log_warn("请求查询结算单失败，原因：{0}", global::mapRetVal(ret));
                }
                return ret;
            };
            qry_queue_->qry<void>(lambda);
        }

        void CtpTradeApi::reqQrySettlementInfo(std::string &dateStr) {
            auto lambda = [this, dateStr](int nRequestID) {
                log_warn("请求查询历史结算单成功");
                CThostFtdcQrySettlementInfoField req;
                memset(&req, 0, sizeof(req));
                strcpy(req.BrokerID, broker_id_.c_str());
                strcpy(req.InvestorID, user_id_.c_str());
                strcpy(req.TradingDay, dateStr.c_str());
                int ret = td_api_->ReqQrySettlementInfo(&req, nRequestID);
                if (ret == 0) {
                    log_debug("nRequestID={0}，请求查询历史结算单成功", nRequestID);
                } else {
                    log_warn("请求查询历史结算单失败，原因：{0}", global::mapRetVal(ret));
                }
                return ret;
            };
            qry_queue_->qry<void>(lambda);
        }
        ///请求查询投资者结算结果响应
        void CtpTradeApi::OnRspQrySettlementInfo(CThostFtdcSettlementInfoField *pSettlementInfo,
                                               CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
            if (pRspInfo && pRspInfo->ErrorID) {
                log_warn("nRequestID={0}，查询投资者结算单失败，原因：[{1}]{2}", nRequestID, pRspInfo->ErrorID, CONVERT_CTP_STR(pRspInfo->ErrorMsg));
                return;
            }
            if (!pSettlementInfo) {
                log_warn("nRequestID={0}，查询投资者结算单pSettlementInfo为空");
                return;
            }
            std::ostringstream oss;
            oss << std::endl << "---------------------nRequestID=" << nRequestID << "，投资者结算单---------------------"
                << std::endl
                << "# 交易日:\t+" << pSettlementInfo->TradingDay << "+" << std::endl
                << "# 结算编号:\t+" << pSettlementInfo->SettlementID << "+" << std::endl
                << "# 经纪公司代码:\t+" << pSettlementInfo->BrokerID << "+" << std::endl
                << "# 投资者代码:\t+" << pSettlementInfo->InvestorID << "+" << std::endl
                << "# 序号:\t+" << pSettlementInfo->SequenceNo << "+" << std::endl
                << "# 消息正文:\t+" << pSettlementInfo->Content << "+" << std::endl;
            log_warn(oss.str());
        }

        void CtpTradeApi::reqSettlementInfoConfirm() {
            auto lambda = [this](int nRequestID) {
                log_warn("请求投资者结算单确认开始");
                CThostFtdcSettlementInfoConfirmField req;
                //memset(&req, 0, sizeof(req));
                strcpy(req.BrokerID, broker_id_.c_str());
                strcpy(req.InvestorID, user_id_.c_str());
//                int nRequestID = qry_queue_->next_req_id();
                int ret = td_api_->ReqSettlementInfoConfirm(&req, nRequestID);
                if (ret == 0) {
                    log_warn("nRequestID={0}，请求投资者结算单确认成功", nRequestID);
                } else {
                    log_warn("请求投资者结算单确认失败，原因：{0}", global::mapRetVal(ret));
                }
                return ret;
            };
            qry_queue_->qry<void>(lambda);
        }

        static const long MAX_SEC_DIFF = 5;
        static void calc_sec_diff(const char *serverDateStr, const char *serverTimeStr) {
            using std::chrono::system_clock;
            using std::chrono::seconds;
            using std::chrono::duration_cast;
            system_clock::time_point now = system_clock::now();
            system_clock::time_point serverDatetime = base::strptimep(std::string(serverDateStr) + " " + serverTimeStr, "%Y%m%d %H:%M:%S");
            long sec_diff = duration_cast<seconds>(serverDatetime - now).count();
            log_info("second diff is {0}", sec_diff);
            if (abs(sec_diff) > MAX_SEC_DIFF) {
//                MY_THROW(base::BaseException, "server and client second diff greater than " + std::to_string(MAX_SEC_DIFF) + "s!");
            }
        }

        ///投资者结算结果确认响应
        void CtpTradeApi::OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm,
                                                   CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                                                   bool bIsLast) {
            if (bIsLast) {
                qry_queue_->on_rsp<void>(nRequestID, nullptr);
            }
            if (pRspInfo && pRspInfo->ErrorID) {
                auto msg = CONVERT_CTP_STR(pRspInfo->ErrorMsg);
                log_warn("nRequestID={0}，确认投资者结算单失败，原因：[{1}]{2}", nRequestID, pRspInfo->ErrorID, msg);
                qry_queue_->on_err(nRequestID, pRspInfo->ErrorID, msg);
                return;
            }
            if (!pSettlementInfoConfirm) {
                log_warn("nRequestID={0}，确认投资者结算单pSettlementInfoConfirm为空", nRequestID);
            }
            calc_sec_diff(pSettlementInfoConfirm->ConfirmDate, pSettlementInfoConfirm->ConfirmTime);
            std::ostringstream oss;
            oss << std::endl << "---------------------nRequestID=" << nRequestID << "，投资者结算结果确认---------------------"
                << std::endl
                << "# 经纪公司代码:\t+" << pSettlementInfoConfirm->BrokerID << "+" << std::endl
                << "# 投资者代码:\t+" << pSettlementInfoConfirm->InvestorID << "+" << std::endl
                << "# 确认日期:\t+" << pSettlementInfoConfirm->ConfirmDate << "+" << std::endl
                << "# 确认时间:\t+" << pSettlementInfoConfirm->ConfirmTime << "+" << std::endl;
            log_warn(oss.str());
        }

        void CtpTradeApi::reqQrySettlementInfoConfirm() {
            auto lambda = [this](int nRequestID) {
                log_warn("查询投资者结算结果确认日期开始");
                CThostFtdcQrySettlementInfoConfirmField req;
                //memset(&req, 0, sizeof(req));
                strcpy(req.BrokerID, broker_id_.c_str());
                strcpy(req.InvestorID, user_id_.c_str());
                int ret = td_api_->ReqQrySettlementInfoConfirm(&req, nRequestID);
                if (ret == 0) {
                    log_debug("nRequestID={0}，查询投资者结算结果确认日期成功", nRequestID);
                } else {
                    log_warn("查询投资者结算结果确认日期失败，原因：{0}", global::mapRetVal(ret));
                }
                return ret;
            };
            qry_queue_->qry<void>(lambda);
        }

        ///请求查询结算信息确认日期响应
        void
        CtpTradeApi::OnRspQrySettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm,
                                                 CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
            if (pRspInfo && pRspInfo->ErrorID) {
                log_warn("nRequestID={0}，确认投资者结算单失败，原因：[{1}]{2}", nRequestID, pRspInfo->ErrorID, CONVERT_CTP_STR(pRspInfo->ErrorMsg));
                return;
            }
            if (!pSettlementInfoConfirm) {
                log_warn("nRequestID={0}，确认投资者结算单pSettlementInfoConfirm为空");
                return;
            }
            log_warn("nRequestID={0}，确认投资者结算单成功", nRequestID);
        }


        void CtpTradeApi::reqQryInvestorPositionDetail() {
            auto lambda = [this](int nRequestID) {
                log_warn("请求查询投资者持仓明细开始");
                CThostFtdcQryInvestorPositionDetailField req;
                memset(&req, 0, sizeof(req));
                strcpy(req.BrokerID, broker_id_.c_str());
                strcpy(req.InvestorID, user_id_.c_str());
                int ret = td_api_->ReqQryInvestorPositionDetail(&req, nRequestID);
                if (ret == 0) {
                    log_debug("nRequestID={0}，请求查询投资者持仓明细成功", nRequestID);
                } else {
                    log_warn("请求查询投资者持仓明细失败，原因：{0}", global::mapRetVal(ret));
                }
                return ret;
            };
            qry_queue_->qry<void>(lambda);
        }

        ///请求查询投资者持仓明细响应
        void CtpTradeApi::OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField *pInvestorPositionDetail,
                                                       CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
            if (pRspInfo && pRspInfo->ErrorID) {
                log_warn("nRequestID={0}，请求查询投资者持仓明细失败", nRequestID);
                return;
            }
            if (!pInvestorPositionDetail) {
                log_warn("nRequestID={0}，请求查询投资者持仓明细pInvestorPositionDetail为空");
                return;
            }
            std::ostringstream oss;
            oss << std::endl << "---------------------nRequestID=" << nRequestID << "，投资者持仓明细---------------------"
                << std::endl
                << "# 合约代码:\t+" << pInvestorPositionDetail->InstrumentID << "+" << std::endl
                << "# 经纪公司代码:\t+" << pInvestorPositionDetail->BrokerID << "+" << std::endl
                << "# 投资者代码:\t+" << pInvestorPositionDetail->InvestorID << "+" << std::endl
                << "# 投机套保标志:\t+" << pInvestorPositionDetail->HedgeFlag << "+" << std::endl
                << "# 买卖:\t+" << pInvestorPositionDetail->Direction << "+" << std::endl
                << "# 开仓日期:\t+" << pInvestorPositionDetail->OpenDate << "+" << std::endl
                << "# 成交编号:\t+" << pInvestorPositionDetail->TradeID << "+" << std::endl
                << "# 数量:\t+" << pInvestorPositionDetail->Volume << "+" << std::endl
                << "# 开仓价:\t+" << pInvestorPositionDetail->OpenPrice << "+" << std::endl
                << "# 交易日:\t+" << pInvestorPositionDetail->TradingDay << "+" << std::endl
                << "# 结算编号:\t+" << pInvestorPositionDetail->SettlementID << "+" << std::endl
                << "# 成交类型:\t+" << pInvestorPositionDetail->TradeType << "+" << std::endl
                << "# 组合合约代码:\t+" << pInvestorPositionDetail->CombInstrumentID << "+" << std::endl
                << "# 交易所代码:\t+" << pInvestorPositionDetail->ExchangeID << "+" << std::endl
                << "# 逐日盯市平仓盈亏:\t+" << pInvestorPositionDetail->CloseProfitByDate << "+" << std::endl
                << "# 逐笔对冲平仓盈亏:\t+" << pInvestorPositionDetail->CloseProfitByTrade << "+" << std::endl
                << "# 逐日盯市持仓盈亏:\t+" << pInvestorPositionDetail->PositionProfitByDate << "+" << std::endl
                << "# 逐笔对冲持仓盈亏:\t+" << pInvestorPositionDetail->PositionProfitByTrade << "+" << std::endl
                << "# 投资者保证金:\t+" << pInvestorPositionDetail->Margin << "+" << std::endl
                << "# 交易所保证金:\t+" << pInvestorPositionDetail->ExchMargin << "+" << std::endl
                << "# 保证金率:\t+" << pInvestorPositionDetail->MarginRateByMoney << "+" << std::endl
                << "# 保证金率(按手数):\t+" << pInvestorPositionDetail->MarginRateByVolume << "+" << std::endl
                << "# 昨结算价:\t+" << pInvestorPositionDetail->LastSettlementPrice << "+" << std::endl
                << "# 结算价:\t+" << pInvestorPositionDetail->SettlementPrice << "+" << std::endl
                << "# 平仓量:\t+" << pInvestorPositionDetail->CloseVolume << "+" << std::endl
                << "# 平仓金额:\t+" << pInvestorPositionDetail->CloseAmount << "+" << std::endl;
            log_warn(oss.str());
        }

        void CtpTradeApi::reqQryNotice() {
            auto lambda = [this](int nRequestID) {
                log_warn("请求查询客户通知开始");
                CThostFtdcQryNoticeField req;
                memset(&req, 0, sizeof(req));
                strcpy(req.BrokerID, broker_id_.c_str());
                int ret = td_api_->ReqQryNotice(&req, nRequestID);
                if (ret == 0) {
                    log_debug("nRequestID={0}，请求查询客户通知成功", nRequestID);
                } else {
                    log_warn("请求查询客户通知失败，原因：{0}", global::mapRetVal(ret));
                }
                return ret;
            };
            qry_queue_->qry<void>(lambda);
        }
        ///请求查询客户通知响应
        void
        CtpTradeApi::OnRspQryNotice(CThostFtdcNoticeField *pNotice, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                                  bool bIsLast) {}

        void CtpTradeApi::reqQryInvestorPositionCombineDetail() {
            auto lambda = [this](int nRequestID) {
                log_warn("请求查询投资者组合持仓明细开始");
                CThostFtdcQryInvestorPositionCombineDetailField req;
                //memset(&req, 0, sizeof(req));
                strcpy(req.BrokerID, broker_id_.c_str());
                strcpy(req.InvestorID, user_id_.c_str());
                int ret = td_api_->ReqQryInvestorPositionCombineDetail(&req, nRequestID);
                if (ret == 0) {
                    log_debug("nRequestID={0}，请求查询投资者组合持仓明细成功", nRequestID);
                } else {
                    log_warn("请求查询投资者组合持仓明细失败，原因：{0}", global::mapRetVal(ret));
                }
                return ret;
            };
            qry_queue_->qry<void>(lambda);
        }

        ///请求查询投资者组持仓明细响应
        void CtpTradeApi::OnRspQryInvestorPositionCombineDetail(
                CThostFtdcInvestorPositionCombineDetailField *pInvestorPositionCombineDetail,
                CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
            if (pRspInfo && pRspInfo->ErrorID) {
                log_warn("nRequestID={0}，请求查询投资者组持仓明细，原因：[{1}]{2}", nRequestID, pRspInfo->ErrorID, CONVERT_CTP_STR(pRspInfo->ErrorMsg));
                return;
            }
            if (!pInvestorPositionCombineDetail) {
                log_warn("nRequestID={0}，请求查询投资者组持仓明细pInvestorPositionCombineDetail为空");
                return;
            }
            std::ostringstream oss;
            oss << std::endl << "---------------------nRequestID=" << nRequestID << "，投资者组合持仓明细---------------------"
                << std::endl
                << "# 交易日:\t+" << pInvestorPositionCombineDetail->TradingDay << "+" << std::endl
                << "# 开仓日期:\t+" << pInvestorPositionCombineDetail->OpenDate << "+" << std::endl
                << "# 交易所代码:\t+" << pInvestorPositionCombineDetail->ExchangeID << "+" << std::endl
                << "# 结算编号:\t+" << pInvestorPositionCombineDetail->SettlementID << "+" << std::endl
                << "# 经纪公司代码:\t+" << pInvestorPositionCombineDetail->BrokerID << "+" << std::endl
                << "# 投资者代码:\t+" << pInvestorPositionCombineDetail->InvestorID << "+" << std::endl
                << "# 组合编号:\t+" << pInvestorPositionCombineDetail->ComTradeID << "+" << std::endl
                << "# 撮合编号:\t+" << pInvestorPositionCombineDetail->TradeID << "+" << std::endl
                << "# 合约代码:\t+" << pInvestorPositionCombineDetail->InstrumentID << "+" << std::endl
                << "# 投机套保标志:\t+" << pInvestorPositionCombineDetail->HedgeFlag << "+" << std::endl
                << "# 买卖:\t+" << pInvestorPositionCombineDetail->Direction << "+" << std::endl
                << "# 持仓量:\t+" << pInvestorPositionCombineDetail->TotalAmt << "+" << std::endl
                << "# 投资者保证金:\t+" << pInvestorPositionCombineDetail->Margin << "+" << std::endl
                << "# 交易所保证金:\t+" << pInvestorPositionCombineDetail->ExchMargin << "+" << std::endl
                << "# 保证金率:\t+" << pInvestorPositionCombineDetail->MarginRateByMoney << "+" << std::endl
                << "# 保证金率(按手数):\t+" << pInvestorPositionCombineDetail->MarginRateByVolume << "+" << std::endl
                << "# 单腿编号:\t+" << pInvestorPositionCombineDetail->LegID << "+" << std::endl
                << "# 单腿乘数:\t+" << pInvestorPositionCombineDetail->LegMultiple << "+" << std::endl
                << "# 组合持仓合约编码:\t+" << pInvestorPositionCombineDetail->CombInstrumentID << "+" << std::endl
                << "# 成交组号:\t+" << pInvestorPositionCombineDetail->TradeGroupID << "+" << std::endl;
            log_warn(oss.str());
        }

        void CtpTradeApi::reqQryParkedOrder() {
            auto lambda = [this](int nRequestID) {
                CThostFtdcQryParkedOrderField req;
                //memset(&req, 0, sizeof(req));
                strcpy(req.BrokerID, broker_id_.c_str());
                strcpy(req.InvestorID, user_id_.c_str());
                int ret = td_api_->ReqQryParkedOrder(&req, nRequestID);
                return ret;
            };
            qry_queue_->qry<void>(lambda);
        }

        ///请求查询预埋单响应
        void
        CtpTradeApi::OnRspQryParkedOrder(CThostFtdcParkedOrderField *pParkedOrder, CThostFtdcRspInfoField *pRspInfo,
                                       int nRequestID, bool bIsLast) {}

        void CtpTradeApi::reqQryParkedOrderAction() {
            auto lambda = [this](int nRequestID) {
                CThostFtdcQryParkedOrderActionField req;
                //memset(&req, 0, sizeof(req));
                strcpy(req.BrokerID, broker_id_.c_str());
                strcpy(req.InvestorID, user_id_.c_str());
                int ret = td_api_->ReqQryParkedOrderAction(&req, nRequestID);
                return ret;
            };
            qry_queue_->qry<void>(lambda);
        }
        ///请求查询预埋撤单响应
        void CtpTradeApi::OnRspQryParkedOrderAction(CThostFtdcParkedOrderActionField *pParkedOrderAction,
                                                         CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                                                         bool bIsLast) {}

        void CtpTradeApi::reqQryTradingNotice() {
            auto lambda = [this](int nRequestID) {
                CThostFtdcQryTradingNoticeField req;
                //memset(&req, 0, sizeof(req));
                strcpy(req.BrokerID, broker_id_.c_str());
                strcpy(req.InvestorID, user_id_.c_str());
                int ret = td_api_->ReqQryTradingNotice(&req, nRequestID);
                return ret;
            };
            qry_queue_->qry<void>(lambda);
        }
        ///请求查询交易通知响应
        void
        CtpTradeApi::OnRspQryTradingNotice(CThostFtdcTradingNoticeField *pTradingNotice, CThostFtdcRspInfoField *pRspInfo,
                                        int nRequestID, bool bIsLast) {}


        int CtpTradeApi::send_order(std::shared_ptr<model::Order> order) {
            order->set_order_key(front_id_, session_id_, next_order_ref());
            log_debug("trade_api.send_order---order: {0}", order->to_string());
            CThostFtdcInputOrderField ord;
            memset(&ord, 0, sizeof(ord));
            strcpy(ord.InstrumentID, Env::instance()->get_instrument(order->order_book_id)->instrument_id.c_str());
            ord.LimitPrice = order->price;
            ord.VolumeTotalOriginal = order->quantity;
            ord.OrderPriceType = order->map_order_price_type_to_ctp();
            ord.Direction = order->map_side_to_ctp();
            ord.CombOffsetFlag[0] = order->map_position_effect_to_ctp();
            strcpy(ord.OrderRef, order->order_id.c_str());
            strcpy(ord.InvestorID, user_id_.c_str());
            strcpy(ord.UserID, user_id_.c_str());
            strcpy(ord.BrokerID, broker_id_.c_str());
            ord.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
            ord.ContingentCondition = THOST_FTDC_CC_Immediately;
            ord.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
            ord.IsAutoSuspend = 0;
            ord.TimeCondition = order->map_time_condition_type_to_ctp();
            ord.VolumeCondition = order->map_volume_condition_type_to_ctp();
            ord.MinVolume = order->min_quantity;
//            ord.UserForceClose = 0;
            int nRequestID = qry_queue_->next_req_id();
            int ret = td_api_->ReqOrderInsert(&ord, nRequestID);
            if (ret == 0) {
                log_debug("nRequestID={0}，报单录入请求成功", nRequestID);
            } else {
                log_warn("nRequestID={0}，报单录入请求失败，原因：{1}", nRequestID, global::mapRetVal(ret));
            }
            return nRequestID;
        }

        ///报单录入请求响应
        void CtpTradeApi::OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo,
                                                int nRequestID, bool bIsLast) {
            if (pRspInfo && pRspInfo->ErrorID) {
                log_warn("nRequestID={0}，orderRef={1}，报单录入请求失败，原因：[{2}]{3}", nRequestID, pInputOrder->OrderRef,
                         pRspInfo->ErrorID, CONVERT_CTP_STR(pRspInfo->ErrorMsg));
                //TODO
                //processErrorOrderStatus(tdApiWrap_->getFrontId(), tdApiWrap_->getSessionId(), pInputOrder->OrderRef, pRspInfo->ErrorID);
                return;
            }
            if (!pInputOrder) {
                log_warn("nRequestID={0}，报单录入请求pInputOrder为空", nRequestID);
                return;
            }
            log_info("nRequestID={0}，报单录入请求成功", nRequestID);
/*            auto order = std::make_shared<model::Order>(pInputOrder, get_front_id(), get_session_id());
            auto event = std::make_shared<event::Event<model::Order>>(event::ORDER_CREATION_REJECT, order);
            event::EventBus::publish_event(event);*/
            auto order = std::make_shared<model::Order>(pInputOrder, front_id_, session_id_);
            gateway_->on_order(order);
        }


        static inline std::string mapOrderSubmitStatus(char statusType) {
            std::string statusStr;
            switch (statusType) {
                case THOST_FTDC_OSS_InsertSubmitted:
                    statusStr = "THOST_FTDC_OSS_InsertSubmitted";
                    break;
                case THOST_FTDC_OSS_CancelSubmitted:
                    statusStr = "THOST_FTDC_OSS_CancelSubmitted";
                    break;
                case THOST_FTDC_OSS_ModifySubmitted:
                    statusStr = "THOST_FTDC_OSS_ModifySubmitted";
                    break;
                case THOST_FTDC_OSS_Accepted:
                    statusStr = "THOST_FTDC_OSS_Accepted";
                    break;
                case THOST_FTDC_OSS_InsertRejected:
                    statusStr = "THOST_FTDC_OSS_InsertRejected";
                    break;
                case THOST_FTDC_OSS_CancelRejected:
                    statusStr = "THOST_FTDC_OSS_CancelRejected";
                    break;
                case THOST_FTDC_OSS_ModifyRejected:
                    statusStr = "THOST_FTDC_OSS_ModifyRejected";
                    break;
                default:
                    statusStr = "THOST_FTDC_OSS_Unknown";
            }
            return statusStr;
        }

        ///报单通知
        void CtpTradeApi::OnRtnOrder(CThostFtdcOrderField *pOrder) {

            std::shared_ptr<model::Order> order = nullptr;
            switch(pOrder->OrderStatus) {
                ///全部成交
                case THOST_FTDC_OST_AllTraded:
                    log_trace("THOST_FTDC_OST_AllTraded, {0}", mapOrderSubmitStatus(pOrder->OrderSubmitStatus));
                    order = std::make_shared<model::Order>(pOrder);
                    gateway_->on_order(order);
                    break;
                    ///部分成交不在队列中
                case THOST_FTDC_OST_PartTradedNotQueueing:
                    log_trace("THOST_FTDC_OST_PartTradedNotQueueing, {0}", mapOrderSubmitStatus(pOrder->OrderSubmitStatus));
                    order = std::make_shared<model::Order>(pOrder);
                    gateway_->on_order(order);
                    break;
                    ///未成交不在队列中
                case THOST_FTDC_OST_NoTradeNotQueueing:
                    log_trace("THOST_FTDC_OST_NoTradeNotQueueing, {0}", mapOrderSubmitStatus(pOrder->OrderSubmitStatus));
                    order = std::make_shared<model::Order>(pOrder);
                    gateway_->on_order(order);
                    break;
                    ///撤单
                case THOST_FTDC_OST_Canceled:
                    log_trace("THOST_FTDC_OST_Canceled, {0}", mapOrderSubmitStatus(pOrder->OrderSubmitStatus));
                    order = std::make_shared<model::Order>(pOrder);
                    gateway_->on_order(order);
                    break;
                    ///部分成交还在队列中
                case THOST_FTDC_OST_PartTradedQueueing:
                    log_trace("THOST_FTDC_OST_PartTradedQueueing, {0}", mapOrderSubmitStatus(pOrder->OrderSubmitStatus));
                    break;
                    ///未成交还在队列中
                case THOST_FTDC_OST_NoTradeQueueing:
                    log_trace("THOST_FTDC_OST_NoTradeQueueing, {0}", mapOrderSubmitStatus(pOrder->OrderSubmitStatus));
                    break;
                    ///未知
                case THOST_FTDC_OST_Unknown:
//                    log_trace("THOST_FTDC_OST_Unknown, {0}", mapOrderSubmitStatus(pOrder->OrderSubmitStatus));
                    break;
                    ///尚未触发
                case THOST_FTDC_OST_NotTouched:
                    log_trace("THOST_FTDC_OST_NotTouched, {0}", mapOrderSubmitStatus(pOrder->OrderSubmitStatus));
                    break;
                    ///已触发
                case THOST_FTDC_OST_Touched:
                    log_trace("THOST_FTDC_OST_Touched, {0}", mapOrderSubmitStatus(pOrder->OrderSubmitStatus));
                    break;
            }
        }

        ///成交通知
        void CtpTradeApi::OnRtnTrade(CThostFtdcTradeField *pTrade) {
            auto trade = std::make_shared<model::Trade>(pTrade);
            gateway_->on_trade(trade);
        }

        ///报单录入错误回报
        void
        CtpTradeApi::OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo) {
            if (!pRspInfo) {
                log_warn("报单录入错误回报pRspInfo为空");
                return;
            }
            if (!pInputOrder) {
                log_warn("报单录入错误回报pInputOrder为空");
                return;
            }
            auto order = std::make_shared<model::Order>(pInputOrder, front_id_, session_id_);
            gateway_->on_order(order);
            std::ostringstream oss;
            oss << "报单录入错误回报(CTP校验未通过)，原因：" << CONVERT_CTP_STR(pRspInfo->ErrorMsg) << std::endl;
            oss << std::endl << "---------------------输入报单---------------------" << std::endl
                << "# 经纪公司代码:\t+" << pInputOrder->BrokerID << "+" << std::endl
                << "# 投资者代码:\t+" << pInputOrder->InvestorID << "+" << std::endl
                << "# 合约代码:\t+" << pInputOrder->InstrumentID << "+" << std::endl
                << "# 报单引用:\t+" << pInputOrder->OrderRef << "+" << std::endl
                << "# 用户代码:\t+" << pInputOrder->UserID << "+" << std::endl
                << "# 报单价格条件:\t+" << pInputOrder->OrderPriceType << "+" << std::endl
                << "# 买卖方向:\t+" << pInputOrder->Direction << "+" << std::endl
                << "# 组合开平标志:\t+" << pInputOrder->CombOffsetFlag << "+" << std::endl
                << "# 组合投机套保标志:\t+" << pInputOrder->CombHedgeFlag << "+" << std::endl
                << "# 价格:\t+" << pInputOrder->LimitPrice << "+" << std::endl
                << "# 数量:\t+" << pInputOrder->VolumeTotalOriginal << "+" << std::endl
                << "# 有效期类型:\t+" << pInputOrder->TimeCondition << "+" << std::endl
                << "# GTD日期:\t+" << pInputOrder->GTDDate << "+" << std::endl
                << "# 成交量类型:\t+" << pInputOrder->VolumeCondition << "+" << std::endl
                << "# 最小成交量:\t+" << pInputOrder->MinVolume << "+" << std::endl
                << "# 触发条件:\t+" << pInputOrder->ContingentCondition << "+" << std::endl
                << "# 止损价:\t+" << pInputOrder->StopPrice << "+" << std::endl
                << "# 强平原因:\t+" << pInputOrder->ForceCloseReason << "+" << std::endl
                << "# 自动挂起标志:\t+" << pInputOrder->IsAutoSuspend << "+" << std::endl
                << "# 业务单元:\t+" << pInputOrder->BusinessUnit << "+" << std::endl
                << "# 请求编号:\t+" << pInputOrder->RequestID << "+" << std::endl
                << "# 用户强评标志:\t+" << pInputOrder->UserForceClose << "+" << std::endl
                << "# 互换单标志:\t+" << pInputOrder->IsSwapOrder << "+" << std::endl;
            log_warn(oss.str());
        }

        void CtpTradeApi::reqOrderParkedInsert(CThostFtdcParkedOrderField &req) {
            //CThostFtdcParkedOrderField req;
            //memset(&req, 0, sizeof(req));
            ///经纪公司代码
            strcpy(req.BrokerID, broker_id_.c_str());
            ///投资者代码
            strcpy(req.InvestorID, user_id_.c_str());
            ///合约代码
            //strcpy(req.InstrumentID, (char*)task->instrumentId.c_str());
            ///报单引用
            //strcpy(req.OrderRef, IntToStr(++m_orderRef).c_str());
            ///用户代码
            //TThostFtdcUserIDType	UserID;
            ///报单价格条件: 限价
            req.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
            ///买卖方向
            //req.Direction = task->direction;
            ///组合开平标志: 开仓
            //req.CombOffsetFlag[0] = task->combOffsetFlag;
            ///组合投机套保标志
            //req.CombHedgeFlag[0] = task->combHedgeFlag;
            ///价格
            //req.LimitPrice = task->price;
            ///数量: volume
            //req.VolumeTotalOriginal = task->total_volume;
            ///有效期类型: 当日有效
            req.TimeCondition = THOST_FTDC_TC_GFD;
            ///GTD日期
            //TThostFtdcDateType	GTDDate;
            ///成交量类型: 任何数量
            req.VolumeCondition = THOST_FTDC_VC_AV;
            ///最小成交量: 1
            req.MinVolume = 1;
            ///触发条件: 立即
            req.ContingentCondition = THOST_FTDC_CC_ParkedOrder;
            ///止损价
            //TThostFtdcPriceType	StopPrice;
            ///强平原因: 非强平
            req.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
            ///自动挂起标志: 是
            req.IsAutoSuspend = 1;
            ///业务单元
            //TThostFtdcBusinessUnitType	BusinessUnit;
            ///请求编号
            //TThostFtdcRequestIDType	RequestID;
            ///用户强评标志: 否
            req.UserForceClose = 0;
            int ret = td_api_->ReqParkedOrderInsert(&req, qry_queue_->next_req_id());

        }
        ///预埋单录入请求响应
        void
        CtpTradeApi::OnRspParkedOrderInsert(CThostFtdcParkedOrderField *pParkedOrder, CThostFtdcRspInfoField *pRspInfo,
                                          int nRequestID, bool bIsLast) {}

        int CtpTradeApi::cancel_order(std::shared_ptr<model::Order> order) {
            CThostFtdcInputOrderActionField ord;
            memset(&ord, 0, sizeof(ord));
            strcpy(ord.InstrumentID, order->order_book_id.c_str());
//            ord.ExchangeID = ???  //TODO
            strcpy(ord.OrderRef, order->order_id.c_str());
            ord.FrontID = order->front_id;
            ord.SessionID = order->session_id;
            ord.ActionFlag = THOST_FTDC_AF_Delete;
            strcpy(ord.BrokerID, broker_id_.c_str());
            strcpy(ord.InvestorID, user_id_.c_str());
            int nRequestID = qry_queue_->next_req_id();
            int ret = td_api_->ReqOrderAction(&ord, nRequestID);
            if (ret == 0) {
                log_warn("nRequestID={0}，报单录入请求成功", nRequestID);
            } else {
                log_warn("nRequestID={0}，报单录入请求失败，原因：{1}", nRequestID, global::mapRetVal(ret));
            }
            return nRequestID;
        }

        ///报单操作请求响应
        void
        CtpTradeApi::OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo,
                                    int nRequestID, bool bIsLast) {
            if (pRspInfo && pRspInfo->ErrorID) {
                log_warn("nRequestID={0}，报单操作请求失败，原因：[{2}]{3}", nRequestID,
                         pRspInfo->ErrorID, CONVERT_CTP_STR(pRspInfo->ErrorMsg));
                return;
            }
            if (!pInputOrderAction) {
                log_warn("nRequestID={0}，报单操作请求pInputOrderAction为空", nRequestID);
                return;
            }
            log_info("nRequestID={0}，报单操作请求成功", nRequestID);
        }
        ///报单操作错误回报
        void
        CtpTradeApi::OnErrRtnOrderAction(CThostFtdcOrderActionField *pOrderAction, CThostFtdcRspInfoField *pRspInfo) {}


        void CtpTradeApi::reqParkedOrderAction(CThostFtdcParkedOrderActionField &req) {
            //CThostFtdcParkedOrderActionField req;
            //memset(&req, 0, sizeof(req));
            ///经纪公司代码
            strcpy(req.BrokerID, broker_id_.c_str());
            ///投资者代码
            strcpy(req.InvestorID, user_id_.c_str());
            ///交易所代码
            //TThostFtdcExchangeIDType	ExchangeID;
            //strcpy(req.ExchangeID, (char*)task->exchangeId.c_str());
            ///报单编号
            //strcpy(req.OrderSysID, (char*)(task->orderSysId.c_str()));
            ///操作标志
            req.ActionFlag = THOST_FTDC_AF_Delete;
            ///价格
            //TThostFtdcPriceType	LimitPrice;
            ///数量变化
            //TThostFtdcVolumeType	VolumeChange;
            ///用户代码
            //TThostFtdcUserIDType	UserID;
            ///合约代码
            //strcpy(req.InstrumentID, (char*)task->instrumentId.c_str());
            int ret = td_api_->ReqParkedOrderAction(&req, qry_queue_->next_req_id());
        }
        ///预埋撤单录入请求响应
        void CtpTradeApi::OnRspParkedOrderAction(CThostFtdcParkedOrderActionField *pParkedOrderAction,
                                               CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}

        ///查询最大报单数量请求
        void CtpTradeApi::reqQueryMaxOrderVolume(CThostFtdcQueryMaxOrderVolumeField *pQueryMaxOrderVolume, int nRequestID) {}
        ///查询最大报单数量响应
        void CtpTradeApi::OnRspQueryMaxOrderVolume(CThostFtdcQueryMaxOrderVolumeField *pQueryMaxOrderVolume, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}

        void CtpTradeApi::reqQryDepthMarketData(const std::string &instrumentId) {

            auto lambda = [this, instrumentId](int nRequestID) {
                log_trace("请求查询深度行情开始");
                CThostFtdcQryDepthMarketDataField req;
                strcpy(req.InstrumentID, instrumentId.c_str());
                int ret = td_api_->ReqQryDepthMarketData(&req, nRequestID);
                if (ret == 0) {
                    log_trace("nRequestID={0}，请求查询深度行情成功", nRequestID);
                } else {
                    log_warn("请求查询深度行情失败，原因：{0}", global::mapRetVal(ret));
                }
                return ret;
            };
            qry_queue_->qry<void>(lambda);
        }

        ///请求查询行情响应
        void CtpTradeApi::OnRspQryDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData,
                                                       CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
        }

        ///请求查询合约保证金率
        void CtpTradeApi::reqQryInstrumentMarginRate(const std::string &instrumentId) {
            auto lambda = [this, instrumentId](int nRequestID) {
                log_trace("请求查询合约保证金率开始");
                CThostFtdcQryInstrumentMarginRateField req;
                strcpy(req.BrokerID, broker_id_.c_str());
                strcpy(req.InvestorID, user_id_.c_str());
                strcpy(req.InstrumentID, instrumentId.c_str());
                req.HedgeFlag = THOST_FTDC_HF_Speculation;
                int ret = td_api_->ReqQryInstrumentMarginRate(&req, nRequestID);
                if (ret == 0) {
                    log_trace("nRequestID={0}，请求查询合约保证金率成功", nRequestID);
                } else {
                    log_warn("请求查询合约保证金率失败，原因：{0}", global::mapRetVal(ret));
                }
                return ret;
            };
            qry_queue_->qry<void>(lambda);
        }
        ///请求查询合约保证金率响应
        void CtpTradeApi::OnRspQryInstrumentMarginRate(CThostFtdcInstrumentMarginRateField *pInstrumentMarginRate,
                                                     CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
            if (pRspInfo && pRspInfo->ErrorID) {
                log_warn("nRequestID={0}，请求查询合约保证金失败，原因：{1}", nRequestID, CONVERT_CTP_STR(pRspInfo->ErrorMsg));
                return;
            }
            if (!pInstrumentMarginRate) {
                log_warn("nRequestID={0}，请求查询合约保证金率pInstrumentMarginRate为空");
                return;
            }
            // trade::service::PositionManager::instance()->addPosition(pInvestorPosition->InstrumentID, pInvestorPosition->PosiDirection,
            // 			pInvestorPosition->Position, pInvestorPosition->TodayPosition);
            if (bIsLast) {
                // trade::service::PositionManager::instance()->updatePositionToRedis();
            }
            std::ostringstream oss;
            oss << std::endl << "---------------------nRequestID=" << nRequestID << "，合约保证金率---------------------"
                << std::endl
                << "# 合约代码:\t+" << pInstrumentMarginRate->InstrumentID << "+" << std::endl
                << "# 投资者范围:\t+" << pInstrumentMarginRate->InvestorRange << "+" << std::endl
                << "# 经纪公司代码:\t+" << pInstrumentMarginRate->BrokerID << "+" << std::endl
                << "# 投资者代码:\t+" << pInstrumentMarginRate->InvestorID << "+" << std::endl
                << "# 投机套保标志:\t+" << pInstrumentMarginRate->HedgeFlag << "+" << std::endl
                << "# 多头保证金率:\t+" << pInstrumentMarginRate->LongMarginRatioByMoney << "+" << std::endl
                << "# 多头保证金费:\t+" << pInstrumentMarginRate->LongMarginRatioByVolume << "+" << std::endl
                << "# 空头保证金率:\t+" << pInstrumentMarginRate->ShortMarginRatioByMoney << "+" << std::endl
                << "# 空头保证金费:\t+" << pInstrumentMarginRate->ShortMarginRatioByVolume << "+" << std::endl
                << "# 是否相对交易所收取:\t+" << pInstrumentMarginRate->IsRelative << "+" << std::endl;
            log_warn(oss.str());
        }

        ///请求查询合约手续费率
        model::CommissionMapPtr CtpTradeApi::qry_commission(const std::string &instrument_id) {
            auto lambda = [this, instrument_id](int nRequestID) {
                log_trace("请求查询合约手续费率开始");
                CThostFtdcQryInstrumentCommissionRateField req;
//                memset(&req, 0, sizeof(req));
                strcpy(req.BrokerID, broker_id_.c_str());
                strcpy(req.InvestorID, user_id_.c_str());
                strcpy(req.InstrumentID, instrument_id.c_str());
                int ret = td_api_->ReqQryInstrumentCommissionRate(&req, nRequestID);
                if (ret == 0) {
                    log_trace("nRequestID={0}，请求查询合约手续费率成功", nRequestID);
                } else {
                    log_warn("请求查询合约手续费率失败，原因：{0}", global::mapRetVal(ret));
                }
                return ret;
            };
            return qry_queue_->qry<model::CommissionMap>(lambda);
        }

        ///请求查询合约手续费率响应
        void
        CtpTradeApi::OnRspQryInstrumentCommissionRate(CThostFtdcInstrumentCommissionRateField *pInstrumentCommissionRate,
                                                    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {

            if (pRspInfo && pRspInfo->ErrorID) {
                auto msg = CONVERT_CTP_STR(pRspInfo->ErrorMsg);
                log_warn("nRequestID={0}，请求查询合约手续费率失败，原因：[{1}]{2}", nRequestID, pRspInfo->ErrorID, msg);
                qry_queue_->on_err(nRequestID, pRspInfo->ErrorID, msg);
                return;
            }
            if (!commission_cache_) {
                commission_cache_ = std::make_shared<model::CommissionMap>();
            }
            if (!pInstrumentCommissionRate) {
                log_warn("nRequestID={0}，请求查询合约手续费率pInstrumentCommissionRate为空");
                qry_queue_->on_rsp(nRequestID, commission_cache_);
                return;
            }
            auto commission = std::make_shared<model::Commission>(pInstrumentCommissionRate);
            (*commission_cache_)[commission->order_book_id] = commission;
            if (bIsLast) {
                qry_queue_->on_rsp(nRequestID, commission_cache_);
                commission_cache_ = nullptr;
            }

            std::ostringstream oss;
            oss << std::endl << "---------------------nRequestID=" << nRequestID << "，合约手续费率---------------------"
                << std::endl
                << "# 合约代码:\t+" << pInstrumentCommissionRate->InstrumentID << "+" << std::endl
                << "# 投资者范围:\t+" << pInstrumentCommissionRate->InvestorRange << "+" << std::endl
                << "# 经纪公司代码:\t+" << pInstrumentCommissionRate->BrokerID << "+" << std::endl
                << "# 投资者代码:\t+" << pInstrumentCommissionRate->InvestorID << "+" << std::endl
                << "# 开仓手续费率:\t+" << pInstrumentCommissionRate->OpenRatioByMoney << "+" << std::endl
                << "# 开仓手续费:\t+" << pInstrumentCommissionRate->OpenRatioByVolume << "+" << std::endl
                << "# 平仓手续费率:\t+" << pInstrumentCommissionRate->CloseRatioByMoney << "+" << std::endl
                << "# 平仓手续费:\t+" << pInstrumentCommissionRate->CloseRatioByVolume << "+" << std::endl
                << "# 平今手续费率:\t+" << pInstrumentCommissionRate->CloseTodayRatioByMoney << "+" << std::endl
                << "# 平今手续费:\t+" << pInstrumentCommissionRate->CloseTodayRatioByVolume << "+" << std::endl;
            log_warn(oss.str());
        }

        ///错误应答
        void CtpTradeApi::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
            if (pRspInfo && pRspInfo->ErrorID) {
                log_warn("nRequestID={0}，交易服务，CTP前置返回错误，原因：[{1}]{2}", nRequestID, pRspInfo->ErrorID, CONVERT_CTP_STR(pRspInfo->ErrorMsg));
                return;
            }
            log_critical("nRquestID={0}，未知错误应答！！", nRequestID);
        }

    } /* ctp */
} /* iqt */ 
