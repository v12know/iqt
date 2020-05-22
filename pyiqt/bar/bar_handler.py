# -*- coding: utf-8 -*-
import pandas as pd

from pyiqt.bar.time_validator import TimeValidator
from pyiqt.data.phasor_data_source import PhasorDataSource
from pyiqt.environment import Environment


class BarHandler(object):
    def __init__(self, order_book_id, bar_count, frequency, fields, include_now=True):
        self._order_book_id = order_book_id
        self._bar_count = bar_count
        self._frequency = frequency
        self._fields = fields
        self._include_now = include_now
        ds = PhasorDataSource()
        self._bar_line = ds.history_bars(order_book_id, bar_count, frequency, fields, include_now=include_now)
        self._bar_line: pd.DataFrame = self._bar_line.sort_index()
        self._last_volume = 0
        self._last_turnover = 0.
        self._init_flag = False
        self._trading_date = Environment.trading_date
        self._listeners = []

    def add_listener(self, l):
        self._listeners.append(l)

    def handle_bar(self, new_bar):
        new_bar["datetime"] = TimeValidator().adjust_time(self._frequency, self._order_book_id, new_bar["datetime"],
                                                          self._trading_date)
        old_bar = self._bar_line[-1]
        if not self._init_flag:
            if "volume" in self._fields:
                self._last_volume = new_bar["volume"]
            if "total_turnover" in self._fields:
                self._last_turnover = new_bar["turnover"]
            self._init_flag = True
        if old_bar["datetime"] == new_bar["datetime"]:
            if "close" in self._fields:
                old_bar["close"] = new_bar["close"]
            if "low" in self._fields:
                old_bar["low"] = min(new_bar["low"], old_bar["low"])
            if "high" in self._fields:
                old_bar["high"] = max(new_bar["high"], old_bar["high"])
            if "volume" in self._fields:
                old_bar["volume"] = new_bar["volume"]
            if "total_turnover" in self._fields:
                old_bar["total_turnover"] = new_bar["total_turnover"]
            if "open_interest" in self._fields:
                old_bar["open_interest"] = new_bar["open_interest"]
        elif old_bar["datetime"] < new_bar["datetime"]:
            if "volume" in self._fields:
                self._last_volume = old_bar["volume"]
            if "total_turnover" in self._fields:
                self._last_turnover = old_bar["turnover"]
            self._bar_line.append(new_bar)
            self._bar_line.drop(index=0)
            for l in self._listeners:
                l(new_bar)


if __name__ == '__main__':
    pass
