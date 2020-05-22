# -*- coding: utf-8 -*-

import redis
import sys

stgy_id = "option_grid_ma1709_buy2b"  # 策略id
side = "L"  # long or short
reset_diff_volume = 7  # 需要减少的手数
diff_step_volume = 2  # 每格容纳的手数
base_price = 2386  # 基准价格
price_tick = 5  # 每格的价格点数
update_flag = False  # 是否更新数据

if __name__ == '__main__':
    if len(sys.argv) >= 2:
        stgy_id = sys.argv[1]
    r = redis.StrictRedis(host='localhost', port=6379, db=0)
    max_rpt_id = 0
    for (version, rpt_id) in r.zrange("stgy:" + stgy_id, 0, -1, withscores=True):
        print(version)
        if max_rpt_id < rpt_id:
            max_rpt_id = int(rpt_id)
    print(max_rpt_id)
    cur_txn_id = r.get("rpt:{}:txn:{}".format(max_rpt_id, side))
    print("txn_id", cur_txn_id)
    diff_open_key = "{}:D:O".format(cur_txn_id.decode())
    diff_close_key = "{}:D:C".format(cur_txn_id.decode())
    diff_opening_key = "{}:D:O:ing".format(cur_txn_id.decode())
    diff_closing_key = "{}:D:C:ing".format(cur_txn_id.decode())

    diff_open_map = r.hgetall(diff_open_key)
    print(diff_open_map)
    total_open = 0
    total_step_num = 0
    for key, value in diff_open_map.items():
        total_step_num += 1
        if int(value) == 0:
            continue
        total_open += int(value)
    print("total open volume is %d" % total_open)
    diff_open_map = {}
    diff_close_map = {}
    diff_opening_map = {}
    diff_closing_map = {}

    remain_diff_volume = total_open - reset_diff_volume
    for i in range(total_step_num):
        diff_opening_map["diff{}".format(i)] = 0
        diff_closing_map["diff{}".format(i)] = 0
        if remain_diff_volume > diff_step_volume:
            diff_open_map["diff{}".format(i)] = str(diff_step_volume)
            diff_close_map["diff{}".format(i)] = 0
        elif remain_diff_volume > 0:
            diff_open_map["diff{}".format(i)] = str(remain_diff_volume)
            diff_close_map["diff{}".format(i)] = str(diff_step_volume - remain_diff_volume)
        else:
            diff_open_map["diff{}".format(i)] = "0"
            diff_close_map["diff{}".format(i)] = str(diff_step_volume)

        remain_diff_volume -= diff_step_volume

    print(diff_open_map)
    print(diff_close_map)

    if diff_open_map:
        r.hmset(diff_open_key, diff_open_map)
        r.hmset(diff_close_key, diff_close_map)
        r.hmset(diff_opening_key, diff_opening_map)
        r.hmset(diff_closing_key, diff_closing_map)

    txn_cost_key = "{}:cost".format(cur_txn_id.decode())
    reset_diff_volume2 = reset_diff_volume
    if update_flag:
        for (hedge_id, avg_cost) in r.zrevrange(txn_cost_key, 0, -1, withscores=True):
            print(r.hgetall(hedge_id)[b"remainUnitVol"])
            if reset_diff_volume2 <= 0:
                break
            remain_unit_vol = int(r.hgetall(hedge_id)[b"remainUnitVol"])
            if reset_diff_volume2 >= remain_unit_vol:
                r.zrem(txn_cost_key, hedge_id)
            else:
                r.hmset(hedge_id, {"remainingUnitVol": remain_unit_vol - reset_diff_volume2,
                                   "remainUnitVol": remain_unit_vol - reset_diff_volume2})
            reset_diff_volume2 -= remain_unit_vol

        txn_price_key = "longMaxPrice" if side == "L" else "shortMinPrice"
        if total_open >= reset_diff_volume:
            import math

            base_price1 = base_price + math.ceil((total_open - reset_diff_volume) / diff_step_volume) * price_tick * (
            1 if side == "L" else -1)
            r.hmset(cur_txn_id, {txn_price_key: base_price1})
