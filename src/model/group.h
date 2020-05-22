#ifndef _IQT_MODEL_GROUP_H_
#define _IQT_MODEL_GROUP_H_

#include <vector>

//#include "model/order.h"

namespace iqt {
    namespace model {
        struct Txn;
        struct Order;

        typedef std::shared_ptr<Txn> TxnPtr;
        typedef std::shared_ptr<Order> OrderPtr;

        namespace group {
            static const std::string GROUP = "group:";
            static const std::string OPEN_GROUP = "open_group:";
            static const std::string CLOSE_GROUP = "close_group:";
            static const std::string TXN_KEY = "txn_key";
            static const std::string UNIT_QUANTITY = "unit_quantity";
            static const std::string FILLED_UNIT_QUANTITY = "filled_unit_quantity";
            static const std::string AVG_COST = "avg_cost";
            static const std::string TRANSACTION_COST = "transaction_cost";
            static const std::string CLOSE_UNIT_QUANTITY = "close_unit_quantity";
            static const std::string CLOSING_UNIT_QUANTITY = "closing_unit_quantity";
            static const std::string SLOT_INDEX = "slot_index";
            static const std::string OPEN_GROUP_KEYS = "open_group_keys";
            static const std::string GROUP_TYPE = "group_type";
            static const std::string REALIZED_PNL = "realized_pnl";

            enum class GROUP_TYPE_TYPE {
                NORMAL,
                LOCK
            };
            static base::EnumWrap<GROUP_TYPE_TYPE, std::string> s_group_type_type({"NORMAL", "LOCK"});
        }

        namespace leg {
            static const std::string LEG = "leg";
            static const std::string FILLED_QUANTITY = "_filled_quantity";
            static const std::string AVG_PRICE = "_avg_price";
            static const std::string TRANSACTION_COST = "_transaction_cost";
            static const std::string FRAG_STATUS = "_frag_status";
            static const std::string ORDER_KEYS = "_order_keys";


            enum class FRAG_STATUS_TYPE {
                NO,
                NEED,
                RCV_ONE,
                RCV_TWO
            };

            static base::EnumWrap<FRAG_STATUS_TYPE, std::string> s_frag_status_type({"NO", "NEED", "RCV_ONE", "RCV_TWO"});

        }

        struct Leg {
            int filled_quantity = 0;//已经交易手数
            double avg_price = 0;//平均价格
            double transaction_cost = 0;//交易费用
            leg::FRAG_STATUS_TYPE frag_status;//分片状态（用于一次性发起平昨今仓order），'NO'不需要分片，'NEED'需要分片 'ONE'接收到分片1，'TWO'接收到分片2

            std::vector<OrderPtr> orders;

            std::string to_string() const {
                std::ostringstream oss;
                oss << "filled_quantity=" << filled_quantity << ", avg_price=" << avg_price << ", transaction_cost="
                    << transaction_cost << ", frag_status=" << leg::s_frag_status_type.enum2literal(frag_status);
                return oss.str();
            }
        };
        typedef std::shared_ptr<Leg> LegPtr;

        struct OpenGroup {
            OpenGroup() = default;
/*            OpenGroup(size_t leg_size) {
                for (size_t i = 0; i < leg_size; ++i) {
                    leg_items.push_back(std::make_shared<Leg>());
                }
            }*/
            TxnPtr txn;
            int slot_index;
            std::string group_key;
            int unit_quantity;//打算交易单位手数
            int filled_unit_quantity = 0;//已经交易单位手数
            double avg_cost = 0;//对冲后的平均成本，有可是价差、比价等
            double transaction_cost = 0;//交易费用

            int close_unit_quantity = 0;//已经平仓的单位手数，平仓时用到
            int closing_unit_quantity = 0;//正在平仓的单位手数，平仓时用到

            group::GROUP_TYPE_TYPE group_type = group::GROUP_TYPE_TYPE::NORMAL;

            std::vector<LegPtr> leg_items;

            std::string to_string() const {
                std::ostringstream oss;
                oss << "slot_index=" << slot_index << ", group_key=" << group_key << ", unit_quantity="
                    << unit_quantity << ", filled_unit_quantity=" << filled_unit_quantity << ", avg_cost="
                    << avg_cost << ", transaction_cost=" << transaction_cost << ", close_unit_quantity="
                    << close_unit_quantity << ", closing_unit_quantity=" << closing_unit_quantity
                    << ", group_type=" << group::s_group_type_type.enum2literal(group_type);
                for (size_t i = 0; i < leg_items.size(); ++i) {
                    oss << std::endl << "leg_items[" << i << "]:" << std::endl << leg_items[i]->to_string();
                }
                return oss.str();
            }
        };
        typedef std::shared_ptr<OpenGroup> OpenGroupPtr;

/*        struct OpenGroupMark {//已开仓组信息标记
            OpenGroupPtr open_group;
            int closing_unit_quantity;//开仓组对应的正在平仓的单位手数
        };
        typedef std::shared_ptr<OpenGroupMark> OpenGroupMarkPtr;*/
        struct CloseGroup {
            CloseGroup() = default;
/*            CloseGroup(size_t leg_size) {
                for (size_t i = 0; i < leg_size; ++i) {
                    leg_items.push_back(std::make_shared<Leg>());
                }
            }*/

            TxnPtr txn;
            std::string group_key;
            int unit_quantity;//打算交易单位手数
            int filled_unit_quantity = 0;//已经交易单位手数
            double avg_cost;//对冲后的平均成本，有可能是价差、比价等
            double transaction_cost = 0;//交易费用

            double realized_pnl = 0;//平仓盈亏

            std::vector<OpenGroupPtr> open_groups;//已开仓组列表，平仓时用到

            group::GROUP_TYPE_TYPE group_type;

            std::vector<LegPtr> leg_items;

            std::string to_string() const {
                std::ostringstream oss;
                oss << ", group_key=" << group_key << ", unit_quantity="
                    << unit_quantity << ", filled_unit_quantity=" << filled_unit_quantity << ", avg_cost="
                    << avg_cost << ", transaction_cost=" << transaction_cost << ", realized_pnl=" << realized_pnl;
                for (size_t i = 0; i < leg_items.size(); ++i) {
                    oss << std::endl << "leg_items[" << i << "]:" << std::endl << leg_items[i]->to_string();
                }
                return oss.str();
            }
        };

        typedef std::shared_ptr<CloseGroup> CloseGroupPtr;

    } /* model  */

} /* iqt */
#endif /* end of include guard: _IQT_MODEL_GROUP_H_ */
