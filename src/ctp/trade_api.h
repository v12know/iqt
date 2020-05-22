#ifndef _IQT_CTP_TD_API_H_
#define _IQT_CTP_TD_API_H_

#include <string>
#include <atomic>
#include <memory>
#include <unordered_map>

#include "ctp/ThostFtdcTraderApi.h"

namespace iqt {
    namespace trade {
        class QryQueue;
    } /* trade  */

    namespace model {
        struct Instrument;
        struct Commission;
        struct FutureInfo;
        struct Account;
        struct Position;
        struct Order;

        typedef std::unordered_map<std::string, std::shared_ptr<model::Commission>> CommissionMap;
        typedef std::shared_ptr<model::CommissionMap> CommissionMapPtr;

        typedef std::unordered_map<std::string, std::shared_ptr<model::Instrument>> InstrumentMap;
        typedef std::shared_ptr<model::InstrumentMap> InstrumentMapPtr;

        typedef std::unordered_map<std::string, std::shared_ptr<model::FutureInfo>> FutureInfoMap;
        typedef std::shared_ptr<model::FutureInfoMap> FutureInfoMapPtr;

        typedef std::unordered_map<std::string, std::shared_ptr<model::Position>> PositionMap;
        typedef std::shared_ptr<model::PositionMap> PositionMapPtr;

        typedef std::unordered_map<std::string, std::shared_ptr<model::Order>> OrderMap;
        typedef std::shared_ptr<model::OrderMap> OrderMapPtr;
    }
    namespace trade {
// class TdPreload;
// Forward declarations.

        class CtpTradeGateway;

        class CtpTradeApi : public CThostFtdcTraderSpi {
        public:
            friend class CtpTradeGateway;

            CtpTradeApi(CtpTradeGateway *gateway, const std::string &broker_id, const std::string &user_id, const std::string &password,
                      const std::string &address, const std::string &pub_resume_type,
                      const std::string &priv_resume_type);

            virtual ~CtpTradeApi();

            void close();

            void join();

            ///获取当前交易日
            ///@retrun 获取到的交易日
            ///@remark 只有登录成功后,才能得到正确的交易日
            const char *getTradingDay();

            /**
             * @synopsis startTdApi 开始交易接口
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
//            void OnHeartBeatWarning(int nTimeLapse) override {}

            void authenticate();
            ///客户端认证响应
            void OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

            void login();

            ///登录请求响应
            void
            OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                           bool bIsLast) override;

            /**
             * @synopsis reqUserLogout 请求用户登出
             */
            void logout();

            ///登出请求响应
            void
            OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                            bool bIsLast) override;


            /**
             * @synopsis reqQryOrder 请求查询下单
             */
            model::OrderMapPtr qry_order();
            ///请求查询报单响应
            void OnRspQryOrder(CThostFtdcOrderField *pOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

            /**
             * @synopsis reqQryTrade 请求查询成交情况
             */
            void reqQryTrade();
            ///请求查询成交响应
            void OnRspQryTrade(CThostFtdcTradeField *pTrade, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

            /**
             * @synopsis reqQryInvestorPosition 请求查询投资者持仓
             */
            model::PositionMapPtr qry_position();
            ///请求查询投资者持仓响应
            void OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

            /**
            * @synopsis reqQryTradingAccount 请求查询资金账户
            */
            std::shared_ptr<model::Account> qry_account();
            ///请求查询资金账户响应
            void OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

            ///请求查询投资者
            void reqQryInvestor();
            ///请求查询投资者响应
            void OnRspQryInvestor(CThostFtdcInvestorField *pInvestor, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

            ///请求查询交易编码
            void reqQryTradingCode();
            ///请求查询交易编码响应
            void OnRspQryTradingCode(CThostFtdcTradingCodeField *pTradingCode, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

            ///查询交易所
            void reqQryExchange(const std::string &exchangeId);
            ///请求查询交易所响应
            void OnRspQryExchange(CThostFtdcExchangeField *pExchange, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

            ///请求查询合约
            model::InstrumentMapPtr qry_instrument(const std::string &instrument_id = "");
            ///请求查询合约响应
            void OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

            ///查询结算单
            void reqQrySettlementInfo();

            /**
            * @synopsis reqQrySettlementInfo 查询历史结算单
            *
            * @param dateStr 日期字符串，格式：yyyymmdd
            */
            void reqQrySettlementInfo(std::string &dateStr);
            ///请求查询投资者结算结果响应
            void OnRspQrySettlementInfo(CThostFtdcSettlementInfoField *pSettlementInfo, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

            /**
             * @synopsis reqSettlementInfoConfirm 请求确认投资者结算单
             */
            void reqSettlementInfoConfirm();

            ///投资者结算结果确认日期响应
            void OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm,
                                            CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

            ///查询投资者结算结果确认日期
            void reqQrySettlementInfoConfirm();
            ///请求查询结算信息确认响应
            void OnRspQrySettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

            ///请求查询投资者持仓明细
            void reqQryInvestorPositionDetail();
            ///请求查询投资者持仓明细响应
            void OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField *pInvestorPositionDetail, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

            ///请求查询客户通知
            void reqQryNotice();
            ///请求查询客户通知响应
            void OnRspQryNotice(CThostFtdcNoticeField *pNotice, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;


            ///请求查询投资者组合持仓明细
            void reqQryInvestorPositionCombineDetail();
            ///请求查询投资者组合持仓明细响应
            void OnRspQryInvestorPositionCombineDetail(CThostFtdcInvestorPositionCombineDetailField *pInvestorPositionCombineDetail, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

            ///查询预埋单请求
            void reqQryParkedOrder();
            ///请求查询预埋单响应
            void OnRspQryParkedOrder(CThostFtdcParkedOrderField *pParkedOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

            ///查询预埋单撤单请求
            void reqQryParkedOrderAction();
            ///请求查询预埋撤单响应
            void OnRspQryParkedOrderAction(CThostFtdcParkedOrderActionField *pParkedOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

            ///请求查询交易通知
            void reqQryTradingNotice();
            ///请求查询交易通知响应
            void OnRspQryTradingNotice(CThostFtdcTradingNoticeField *pTradingNotice, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

            ///报单录入请求
//            void reqOrderInsert(CThostFtdcInputOrderField &ord);

            int send_order(std::shared_ptr<model::Order> order);

            ///报单录入请求响应
            void
            OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                             bool bIsLast) override;

            ///报单通知
            void OnRtnOrder(CThostFtdcOrderField *pOrder) override;

            ///成交通知
            void OnRtnTrade(CThostFtdcTradeField *pTrade) override;

            ///报单录入错误回报
            void OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo) override;

            ///预埋单录入请求
            void reqOrderParkedInsert(CThostFtdcParkedOrderField &ord);
            ///预埋单录入请求响应
            void OnRspParkedOrderInsert(CThostFtdcParkedOrderField *pParkedOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

            ///报单操作请求
            int cancel_order(std::shared_ptr<model::Order> order);
//            void reqOrderAction(CThostFtdcInputOrderActionField &ord);

            ///报单操作请求响应
            void OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo,
                                  int nRequestID, bool bIsLast) override;

            ///报单操作错误回报
            void
            OnErrRtnOrderAction(CThostFtdcOrderActionField *pOrderAction, CThostFtdcRspInfoField *pRspInfo) override;

            ///预埋单撤单请求
            void reqParkedOrderAction(CThostFtdcParkedOrderActionField &ord);

            ///预埋撤单录入请求响应
            void OnRspParkedOrderAction(CThostFtdcParkedOrderActionField *pParkedOrderAction,
                                        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;


            ///查询最大报单数量请求
            void reqQueryMaxOrderVolume(CThostFtdcQueryMaxOrderVolumeField *pQueryMaxOrderVolume, int nRequestID);
            ///查询最大报单数量响应
            void OnRspQueryMaxOrderVolume(CThostFtdcQueryMaxOrderVolumeField *pQueryMaxOrderVolume, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

            ///请求查询行情
            void reqQryDepthMarketData(const std::string &instrumentId);

            ///请求查询行情响应
            void
            OnRspQryDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData, CThostFtdcRspInfoField *pRspInfo,
                                    int nRequestID, bool bIsLast) override;


            ///请求查询合约保证金率
            void reqQryInstrumentMarginRate(const std::string &instrumentId);

            ///请求查询合约保证金率响应
            void OnRspQryInstrumentMarginRate(CThostFtdcInstrumentMarginRateField *pInstrumentMarginRate,
                                              CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

            ///请求查询合约手续费率
            model::CommissionMapPtr qry_commission(const std::string &instrument_id = "");

            ///请求查询合约手续费率响应
            void OnRspQryInstrumentCommissionRate(CThostFtdcInstrumentCommissionRateField *pInstrumentCommissionRate,
                                                  CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                                                  bool bIsLast) override;

            ///错误应答
            void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

            inline void set_fs_id(const int &frontId, const int &sessionId) {
#ifndef VTEST
                front_id_ = frontId;
                session_id_ = sessionId;
                fs_id_ = std::to_string(front_id_) + ":" + std::to_string(session_id_);
#else
                frontId_ = 0;
                sessionId_ = 0;
                FSId_ = "0:0";
#endif
            }

            inline int get_front_id() {
                return front_id_;
            }

            inline int get_session_id() {
                return session_id_;
            }

            inline void set_order_ref(const std::string &orderRef) {
                order_id_ = std::stoi(orderRef);
            }

            inline std::string get_fs_id() {
                return fs_id_;
            }

            inline std::string next_order_ref() {
                return std::to_string(++order_id_);
            }

        private:
            /* data */
            CThostFtdcTraderApi *td_api_;//ctp交易接口类
            CtpTradeGateway *gateway_;
            trade::QryQueue *qry_queue_;

            std::string broker_id_;
            std::string user_id_;
            std::string password_;
            std::string address_;
            std::string pub_resume_type_;
            std::string priv_resume_type_;
            /* 会话参数 */
            int front_id_;
            int session_id_;
            std::string fs_id_;
            std::atomic_int order_id_;

            model::CommissionMapPtr commission_cache_ = nullptr;
            model::PositionMapPtr pos_cache_ = nullptr;
            model::InstrumentMapPtr ins_cache_ = nullptr;
            model::OrderMapPtr order_cache_ = nullptr;

            // static int sId_;
            bool connected_ = false;
            bool logged_in_ = false;
            bool authenticated_ = false;
            bool require_authentication_ = false;
        };

    } /* ctp */

} /* iqt */
#endif /* end of include guard: _IQT_CTP_TD_API_H_ */
