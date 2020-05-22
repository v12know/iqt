# -*- coding: utf-8 -*-
import datetime


class Tick:
    def __init__(self, ctick):
        self.order_book_id = ctick.order_book_id
        self.datetime = datetime.datetime.strftime(ctick.datetime, '%Y-%m-%d %H:%M:%S.%f')[:-3]
        self.open = ctick.open
        self.last = ctick.last
        self.low = ctick.low
        self.high = ctick.high
        self.prev_close = ctick.prev_close
        self.volume = ctick.volume
        self.total_turnover = ctick.total_turnover
        self.open_interest = ctick.open_interest
        self.prev_settlement = ctick.prev_settlement
        self.b1 = ctick.b1
        self.b1_v = ctick.b1_v
        self.a1 = ctick.a1
        self.a1_v = ctick.a1_v
        self.limit_up = ctick.limit_up
        self.limit_down = ctick.limit_down
        self.trading_date = ctick.trading_date



