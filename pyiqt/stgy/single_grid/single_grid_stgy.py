# -*- coding: utf-8 -*-
import threading

import redis
import json
# from pyiqt.stgy import portal_stgy as stgy
from pyiqt.util.decorator import singleton
from pyiqt.util.logger import LoggerFactory

logger = LoggerFactory().get_logger()


@singleton
class SingleGridStgyHelper:
    def __init__(self):
        pool = redis.ConnectionPool(host='59.41.39.19', port=16380, password='_@HuaFu123456@.', db=0)
        # pool = redis.ConnectionPool(host='192.168.0.203', port=6379, db=0)
        self._r = redis.StrictRedis(connection_pool=pool)
        self._thread = threading.Thread(target=self._run)
        self._stgy_map = {}

    def start(self):
        self._thread.start()

    def _run(self):
        p = self._r.pubsub()
        p.subscribe('C_Signal_20171009')
        while True:
            for item in p.listen():
                # print(item)
                if item['type'] == 'message':

                    # logger.info(item['data'].decode())
                    try:
                        data = json.loads(item['data'].decode())
                        if not data["constract"] in self._stgy_map:
                            continue
                        stgy_list = self._stgy_map[data["constract"]]
                        for stgy in stgy_list:
                            stgy.update_signal(data["ma5_ols"], data["ma20_ols"],
                                               data["ma20_dis"], data["ATR"])
                    except:
                        logger.exception("parse signal json error!", exc_info=True)

    def get_signal(self, order_book_id):
        contract = self.get_rq_contract(order_book_id)
        sig_map = self._r.hgetall("C_Signal_20171009_" + contract)
        logger.info(str(sig_map))
        ma5_ols = float(sig_map[b"ma5_ols"].decode())
        ma20_ols = float(sig_map[b"ma20_ols"].decode())
        ma20_dis = float(sig_map[b"ma20_dis"].decode())
        atr = float(sig_map[b"ATR"].decode())
        return ma5_ols, ma20_ols, ma20_dis, atr

    def register_stgy(self, order_book_id, stgy):
        contract = self.get_rq_contract(order_book_id)
        if contract in self._stgy_map:
            stgy_list = self._stgy_map[contract]
            stgy_list.append(stgy)
        else:
            self._stgy_map[contract] = [stgy]
        logger.info("register stgy succesful!")

    @staticmethod
    def get_rq_contract(order_book_id):
        if len(order_book_id) < 4:
            return None
        if order_book_id[-4] not in '0123456789':
            order_book_id = order_book_id[:2] + '1' + order_book_id[-3:]
        else:
            order_book_id = order_book_id
        return order_book_id.upper()


def get_signal(order_book_id):
    return SingleGridStgyHelper().get_signal(order_book_id)


def register_stgy(order_book_id, stgy):
    SingleGridStgyHelper().register_stgy(order_book_id, stgy)


SingleGridStgyHelper().start()


