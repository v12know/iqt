#ifndef _IQT_EVENT_TYPE_H_
#define _IQT_EVENT_TYPE_H_

#include <string>


namespace iqt {
    namespace event {

        //匹配所有的target_id
        const static std::string ALL_TARGET_ID = "all_target_id";
        //没有匹配的target_id
        const static std::string NONE_TARGET_ID = "none_target_id";
        ///////////////////////////////////////////////////

        //策略证券池发生变化后触发
        const static std::string POST_UNIVERSE_CHANGED = "post_universe_changed";

        //该事件会触发策略的before_trading函数
        //before_trading()
        const static std::string BEFORE_TRADING = "before_trading";

        //该事件会触发策略的after_trading函数
        //after_trading()
        const static std::string AFTER_TRADING = "after_trading";
        //执行after_trading函数后触发
        //post_after_trading()
        const static std::string POST_AFTER_TRADING = "post_after_trading";

        //触发结算事件
        const static std::string SETTLEMENT = "settlement";

        //创建订单
        const static std::string ORDER_PENDING_NEW = "order_pending_new";
        //创建订单成功
        const static std::string ORDER_CREATION_PASS = "order_creation_pass";
        //创建订单失败
        const static std::string ORDER_CREATION_REJECT = "order_creation_reject";
        //创建撤单
        const static std::string ORDER_PENDING_CANCEL = "order_pending_cancel";
        //撤销订单成功
        const static std::string ORDER_CANCELLATION_PASS = "order_cancellation_pass";
        //撤销订单失败
        const static std::string ORDER_CANCELLATION_REJECT = "order_cancellation_reject";
        //订单状态更新
        const static std::string ORDER_UNSOLICITED_UPDATE = "order_unsolicited_update";

        //成交回报
        const static std::string TRADE = "trade";

        //bar
        const static std::string BAR = "bar";

        //tick
        const static std::string TICK = "tick";

    } /* event */
} /* iqt */

#endif /* end of include guard: _IQT_EVENT_TYPE_H_ */
