# -*- coding: utf-8 -*-

import redis
# import sys

host = "localhost"
port = 6379
password = ""
db = 0

# stgy_id = "single_grid_ZC805"  #策略id
# version = "v0.1"
#
# open_quantity = 5
# direct = "LONG"


if __name__ == '__main__':
    # if len(sys.argv) >= 2:
    #     stgy_id = sys.argv[1]
    r = redis.StrictRedis(host=host, port=port, password=password, db=db, decode_responses=True)

    key_list = r.keys("stgy:*")
    for stgy_key in key_list:
        if stgy_key == "stgy:":
            continue

        rpt_key = r.get(stgy_key)
        print(rpt_key)
        rpt = r.hgetall(rpt_key)
        # rpt = r.hgetall(rpt_key)
        print(rpt)
        txn_key_list = [rpt["long_txn_key"], rpt["short_txn_key"]]
        for txn_key in txn_key_list:
            txn = r.hgetall(txn_key)
            #if txn["status"] != "RUNNING":
            #    continue
            open_count = int(txn["open_count"])
            print("open_count={}".format(open_count))
            slot_key = txn_key + ":slot"
            slot_list = r.lrange(slot_key, 0, -1)
            i = 0
            for slot in slot_list:
                open, opening, capcity = [int(item) for item in slot.split(",")]
                if open_count >= capcity:
                    open = capcity
                elif open_count > 0:
                    open = open_count
                else:
                    open = 0

                new_slot = "{},0,{}".format(open, capcity)
                #print("old_slot: {}".format(slot))
                print("new_slot: {}".format(new_slot))
                r.lset(slot_key, i, new_slot)
                open_count -= capcity
                i += 1



