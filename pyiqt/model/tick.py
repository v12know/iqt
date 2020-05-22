# -*- coding: utf-8 -*-
import datetime


class Tick:
    def __init__(self, ctick=None):
        if ctick is not None:
            self._init_by_ctick(ctick)
        else:
            self._init_by_default()

    def _init_by_default(self):
        self.order_book_id = ''
        self.datetime = None
        self.open = 0
        self.last = 0
        self.low = 0
        self.high = 0
        self.prev_close = 0
        self.volume = 0
        self.total_turnover = 0
        self.open_interest = 0
        self.settlement = 0
        self.prev_settlement = 0
        self.b1 = 0
        self.b1_v = 0
        self.a1 = 0
        self.a1_v = 0
        self.limit_up = 0
        self.limit_down = 0
        self.trading_date = None

    def _init_by_ctick(self, ctick):
        self.order_book_id = ctick.order_book_id
        self.datetime = datetime.datetime.strftime(ctick.datetime, '%Y-%m-%d %H:%M:%S.%f')[:-3]
        self.open = round(ctick.open, 5)
        self.last = round(ctick.last, 5)
        self.low = round(ctick.low, 5)
        self.high = round(ctick.high, 5)
        self.prev_close = round(ctick.prev_close, 5)
        self.volume = ctick.volume
        self.total_turnover = round(ctick.total_turnover, 5)
        self.open_interest = round(ctick.open_interest, 5)
        self.settlement = round(ctick.settlement, 5)
        self.prev_settlement = round(ctick.prev_settlement, 5)
        self.b1 = round(ctick.b1, 5)
        self.b1_v = ctick.b1_v
        self.a1 = round(ctick.a1, 5)
        self.a1_v = ctick.a1_v
        self.limit_up = round(ctick.limit_up, 5)
        self.limit_down = round(ctick.limit_down, 5)
        self.trading_date = ctick.trading_date



