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
            if txn["status"] == "LOCK":
                r.hset(txn_key, "status", "UNLOCKING")



