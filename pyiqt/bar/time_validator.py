# -*- coding: utf-8 -*-
from datetime import datetime
from datetime import timedelta
import time
import re

from pyiqt.util.decorator import singleton


@singleton
class TimeValidator(object):
    DEFAULT_FIRST_BAR_TIME = "09:00:00"
    DEFAULT_PERIOD = "09:00:00~10:15:00,10:30:00~11:30:00,13:30:00~15:00:00"

    NIGHT_SHFE_SYMBOLS_GJS = ["AU", "AG"]  # gui jin shu
    NIGHT_SHFE_PERIOD_GJS = "21:00:00~02:30:00,09:00:00~10:15:00,10:30:00~11:30:00,13:30:00~15:00:00"

    NIGHT_SHFE_SYMBOLS_YS = ["CU", "AL", "ZN", "PB", "SN", "NI"]  # you se
    NIGHT_SHFE_PERIOD_YS = "21:00:00~01:00:00,09:00:00~10:15:00,10:30:00~11:30:00,13:30:00~15:00:00"

    NIGHT_SHFE_SYMBOLS_HS = ["RU", "RB", "HC", "BU"]  # hei se
    NIGHT_SHFE_PERIOD_HS = "21:00:00~23:00:00,09:00:00~10:15:00,10:30:00~11:30:00,13:30:00~15:00:00"

    NIGHT_INE_SYMBOLS = ["SC"]
    NIGHT_INE_PERIOD = "21:00:00~02:30:00,09:00:00~10:15:00,10:30:00~11:30:00,13:30:00~15:00:00"

    NIGHT_DCE_SYMBOLS = ["P", "J", "M", "Y", "A", "B", "JM", "I"]
    NIGHT_DCE_PERIOD = "21:00:00~23:30:00,09:00:00~10:15:00,10:30:00~11:30:00,13:30:00~15:00:00"

    NIGHT_CZCE_SYMBOLS = ["SR", "CF", "RM", "MA", "TA", "ZC", "FG", "OI"]
    NIGHT_CZCE_PERIOD = "21:00:00~23:30:00,09:00:00~10:15:00,10:30:00~11:30:00,13:30:00~15:00:00"

    CFFEX_SYMBOLS_GZQH = ["IC", "IF", "IH"]  # gu zhi qi huo
    CFFEX_PERIOD_GZQH = "09:30:00~11:30:00,13:00:00~15:00:00"

    CFFEX_SYMBOLS_GZ = ["T", "TF"]  # guo zhai
    CFFEX_PERIOD_GZ = "09:15:00~11:30:00,13:00:00~15:15:00"

    def build_period_list_map(self, symbol_list, period_str: str):
        origin_period_list = self.build_origin_period_list(period_str)
        trading_period_list = self.build_trading_period_list(origin_period_list)
        for symbol in symbol_list:
            first_bar_time = datetime.strptime(origin_period_list[0][0], "%H:%M:%S")
            self._first_bar_time_map[symbol] = first_bar_time
            origin_period_list.sort(key=lambda x: x[0])
            self._ordered_period_list_map[symbol] = origin_period_list
            self._trading_period_list_map[symbol] = trading_period_list

    @classmethod
    def build_trading_period_list(cls, origin_period_list):
        trading_period_list = []
        start = 0
        night_flag = False
        if origin_period_list[0][0] > origin_period_list[1][0]:
            tmp_time1 = datetime.strptime(origin_period_list[0][0], "%H:%M:%S")
            tmp_time2 = datetime.strptime(origin_period_list[0][1], "%H:%M:%S")
            if origin_period_list[0][0] > origin_period_list[0][1]:
                tmp_time2 += timedelta(days=1)
            trading_period_list.append((tmp_time1, tmp_time2))
            night_flag = True
            start = 1
        for i in range(start, len(origin_period_list)):
            tmp_time1 = datetime.strptime(origin_period_list[i][0], "%H:%M:%S")
            tmp_time2 = datetime.strptime(origin_period_list[i][1], "%H:%M:%S")
            if night_flag:
                tmp_time1 += timedelta(days=1)
                tmp_time2 += timedelta(days=1)
            trading_period_list.append((tmp_time1, tmp_time2))
        return trading_period_list

    @classmethod
    def build_origin_period_list(cls, period_str: str):
        origin_period_list = []
        for item in period_str.split(","):
            time_pair = [i.strip() for i in item.split("~")]
            origin_period_list.append(tuple(time_pair))
        return origin_period_list

    def __init__(self):
        # string [('09:00:00', '10:15:00')]
        self._ordered_period_list_map = {}
        # datetime [(datetime.datetime(1900, 1, 1, 21, 0), datetime.datetime(1900, 1, 2, 2, 30))]
        self._trading_period_list_map = {}
        self._first_bar_time_map = {}

        self._default_ordered_period_list = self.build_origin_period_list(self.DEFAULT_PERIOD)
        self._default_trading_period_list = self.build_trading_period_list(self._default_ordered_period_list)
        self._default_ordered_period_list.sort(key=lambda x: x[0])

        self.build_period_list_map(self.NIGHT_SHFE_SYMBOLS_GJS, self.NIGHT_SHFE_PERIOD_GJS)
        self.build_period_list_map(self.NIGHT_SHFE_SYMBOLS_YS, self.NIGHT_SHFE_PERIOD_YS)
        self.build_period_list_map(self.NIGHT_SHFE_SYMBOLS_HS, self.NIGHT_SHFE_PERIOD_HS)

        self.build_period_list_map(self.NIGHT_INE_SYMBOLS, self.NIGHT_INE_PERIOD)

        self.build_period_list_map(self.NIGHT_DCE_SYMBOLS, self.NIGHT_DCE_PERIOD)

        self.build_period_list_map(self.NIGHT_CZCE_SYMBOLS, self.NIGHT_CZCE_PERIOD)

        self.build_period_list_map(self.CFFEX_SYMBOLS_GZQH, self.CFFEX_PERIOD_GZQH)
        self.build_period_list_map(self.CFFEX_SYMBOLS_GZ, self.CFFEX_PERIOD_GZ)

        self.p = re.compile("^(\D+)\d+$")

        self._time_adjuster_3m = TimeAdjuster("3m", self)
        self._time_adjuster_5m = TimeAdjuster("5m", self)
        self._time_adjuster_10m = TimeAdjuster("10m", self)
        self._time_adjuster_15m = TimeAdjuster("15m", self)
        self._time_adjuster_30m = TimeAdjuster("30m", self)
        self._time_adjuster_60m = TimeAdjuster("60m", self)

        self._time_adjuster_map = {
            "1m": self._adjust_time_1m,
            "3m": self._time_adjuster_3m.adjust_time,
            "5m": self._time_adjuster_5m.adjust_time,
            "10m": self._time_adjuster_10m.adjust_time,
            "15m": self._time_adjuster_15m.adjust_time,
            "30m": self._time_adjuster_30m.adjust_time,
            "60m": self._time_adjuster_60m.adjust_time,
            "1d": self._adjust_time_1d,
        }
        pass

    def get_trading_period(self, universe):
        trading_period = []
        # if DEFAULT_ACCOUNT_TYPE.STOCK.name in accounts:
        #     trading_period += STOCK_TRADING_PERIOD

        for order_book_id in universe:
            # if get_account_type(order_book_id) == DEFAULT_ACCOUNT_TYPE.STOCK.name:
            #     continue
            underlying_symbol = re.match(self.p, order_book_id).group(1)
            tmp_period_list = self._ordered_period_list_map[underlying_symbol] \
                if underlying_symbol in self._ordered_period_list_map else self._default_ordered_period_list
            tmp_last_period = tmp_period_list[-1]
            if tmp_last_period[0] > tmp_last_period[1]:
                tmp_last_period = tmp_period_list.pop()
                tmp_period_list.append((tmp_last_period[0], '23:59:59'))
                tmp_period_list.append(('00:00:00', tmp_last_period[1]))
            trading_period += tmp_period_list

        return self.merge_trading_period(trading_period)

    @classmethod
    def merge_trading_period(cls, trading_period: list):
        result = []
        trading_period.sort(key=lambda x: x[0])
        for time_range in trading_period:
            if result and result[-1][1] >= time_range[0]:
                result[-1] = (result[-1][0], max(result[-1][1], time_range[1]))
            else:
                result.append(time_range)
        return result

    def check_time(self, order_book_id, dt: datetime):
        underlying_symbol = re.match(self.p, order_book_id).group(1)
        return self._check_time(underlying_symbol, dt.strftime("%H:%M:%S"))

    def _check_time(self, underlying_symbol, time_str):
        ordered_peroid_list = self._ordered_period_list_map[underlying_symbol] \
            if underlying_symbol in self._ordered_period_list_map else self._default_ordered_period_list
        valid_flag = False
        if ordered_peroid_list[0][0] > ordered_peroid_list[-1][1] > time_str:
            valid_flag = True
        else:
            watch_flag = False
            for item in ordered_peroid_list:
                if item[0] > time_str:
                    valid_flag = False
                    watch_flag = True
                    break
                if item[1] >= time_str:
                    valid_flag = True
                    watch_flag = True
                    break

            if not watch_flag:
                if ordered_peroid_list[-1][1] < ordered_peroid_list[0][0]:
                    valid_flag = True
                else:
                    valid_flag = False

        return valid_flag

    def adjust_time(self, frequency: str, order_book_id: str, orig_dt: datetime, trading_date: str=None):
        if frequency not in self._time_adjuster_map:
            raise NotImplementedError("wrong frequency parameter, should be in {}"
                                      .format(list(self._time_adjuster_map.keys())))
        symbol = re.match(self.p, order_book_id).group(1)
        return self._time_adjuster_map[frequency](symbol=symbol, orig_dt=orig_dt,
                                                  trading_date=trading_date)

    @classmethod
    def _adjust_time_1d(cls, symbol: str, orig_dt: datetime, trading_date: str):
        return datetime.strptime(trading_date, "%Y%m%d")

    def _adjust_time_1m(self, symbol: str, orig_dt: datetime, trading_date: str):
        dt_plus_1m = orig_dt + timedelta(minutes=1)
        dt_plus_1m = dt_plus_1m.replace(second=0)
        dt_plus_1m_str = dt_plus_1m.strftime("%H:%M:%S")
        dt = orig_dt.replace(second=0)
        dt_str = dt.strftime("%H:%M:%S")
        ordered_period_list = self._ordered_period_list_map[symbol] \
            if symbol in self._ordered_period_list_map else self._default_ordered_period_list

        for item in ordered_period_list:
            if item[0] == dt_plus_1m_str:
                dt_plus_1m += timedelta(minutes=1)
                return dt_plus_1m.replace(second=0, microsecond=0)
            if item[1] == dt_str:
                return dt.replace(second=0, microsecond=0)
        dt += timedelta(milliseconds=500, minutes=1)
        return dt.replace(second=0, microsecond=0)

    @property
    def default_trading_period_list(self):
        return self._default_trading_period_list

    @property
    def trading_period_list_map(self):
        return self._trading_period_list_map


class TimeAdjuster:
    def __init__(self, frequency: str, time_validator: TimeValidator):
        self._interval = self._get_interval(frequency)
        self._time_validator = time_validator

        self._default_ordered_period_divide_list = self.build_ordered_period_divide_list(
            self._time_validator.default_trading_period_list)

        self._ordered_period_divide_list_map = {}
        self.build_ordered_period_divide_list_map()

    @classmethod
    def _get_interval(cls, frequency: str):
        if frequency[-1] == "m":
            return int(frequency[:-1])
        if frequency[-1] == "h":
            return int(frequency[:-1]) * 60
        if frequency[-1] == "d":
            return 0

    def build_ordered_period_divide_list(self, trading_period_list):
        origin_period_divide_list = []
        remain_minutes = 0
        begin_dt = None
        end_dt = None
        for pair in trading_period_list:
            begin_time = pair[0]
            end_time = pair[1]
            end_dt = end_time
            end_minutes = int(time.mktime(end_dt.timetuple()) / 60)
            if remain_minutes == 0:
                begin_dt = begin_time
            else:
                tmp_begin_dt = begin_time
                begin_minutes = int(time.mktime(tmp_begin_dt.timetuple()) / 60)
                if begin_minutes + self._interval - remain_minutes <= end_minutes:
                    tmp_begin_dt += timedelta(minutes=self._interval - remain_minutes)
                    origin_period_divide_list.append((begin_dt.strftime("%H:%M:%S"), tmp_begin_dt.strftime("%H:%M:%S")))
                    begin_dt = tmp_begin_dt
                remain_minutes = 0

            begin_minutes = int(time.mktime(begin_dt.timetuple()) / 60)
            while begin_minutes + self._interval <= end_minutes:
                cur_time = begin_dt
                begin_dt += timedelta(minutes=self._interval)
                next_time = begin_dt
                origin_period_divide_list.append((cur_time.strftime("%H:%M:%S"), next_time.strftime("%H:%M:%S")))
                begin_minutes = int(time.mktime(begin_dt.timetuple()) / 60)
            remain_minutes += end_minutes - begin_minutes
        if remain_minutes > 0:
            origin_period_divide_list.append((begin_dt.strftime("%H:%M:%S"), end_dt.strftime("%H:%M:%S")))
        ordered_period_divide_list = []
        sp_pair = None
        origin_period_divide_list.sort(key=lambda x: x[0])
        for pair in origin_period_divide_list:
            if pair[0] > pair[1]:
                sp_pair = pair
            else:
                ordered_period_divide_list.append(pair)
        if sp_pair is not None:
            ordered_period_divide_list.append(sp_pair)
        return ordered_period_divide_list

    def build_ordered_period_divide_list_map(self):
        for key, value in self._time_validator.trading_period_list_map.items():
            ordered_period_divide_list = self.build_ordered_period_divide_list(value)
            self._ordered_period_divide_list_map[key] = ordered_period_divide_list

    def adjust_time(self, symbol: str, orig_dt: datetime, trading_date: str):
        time_str = orig_dt.strftime("%H:%M:%S")
        date_str = orig_dt.strftime("%Y-%m-%d")

        ordered_period_divide_list = self._ordered_period_divide_list_map[symbol]\
            if symbol in self._ordered_period_divide_list_map else self._default_ordered_period_divide_list
        last_period_divide = ordered_period_divide_list[-1]
        max = len(ordered_period_divide_list)
        if last_period_divide[0] > last_period_divide[1]:
            if time_str > last_period_divide[0]:
                orig_dt += timedelta(days=1)
                return datetime.strptime("{} {}".format(orig_dt.strftime("%Y-%m-%d"), last_period_divide[1]),
                                         "%Y-%m-%d %H:%M:%S")
            if time_str <= last_period_divide[1]:
                return datetime.strptime("{} {}".format(date_str, last_period_divide[1]),
                                         "%Y-%m-%d %H:%M:%S")
            max -= 1
        for i in range(max):
            if ordered_period_divide_list[i][0] < time_str <= ordered_period_divide_list[i][1]:
                return datetime.strptime("{} {}".format(date_str, ordered_period_divide_list[i][1]),
                                         "%Y-%m-%d %H:%M:%S")
        raise RuntimeError("can't adjust time! symbol={}, orig_dt={}".format(symbol, orig_dt))


if __name__ == '__main__':
    tv = TimeValidator()
    print(tv.get_trading_period(['WR1805', 'RB1805', 'ZC1805', 'T1809', 'IC1809', 'CU1806']))
    # new_dt = tv.adjust_time("3m", "WR1805", datetime.now(), "20180403")
    # print(new_dt)
    # new_dt = tv.adjust_time("3m", "RB1805", datetime.now(), "20180403")
    # print(new_dt)
    # new_dt = tv.adjust_time("1m", "RB1805", datetime.now(), "20180403")
    # print(new_dt)
    new_dt = tv.adjust_time("1m", "AU1805", datetime.strptime('20180512235959', '%Y%m%d%H%M%S'), "20180403")
    print(new_dt)
    print(tv.check_time('AU1806', datetime.strptime('20180512235958', '%Y%m%d%H%M%S')))
    # new_dt = tv.adjust_time("5m", "RB1805", datetime.now(), "20180403")
    # print(new_dt)
    # new_dt = tv.adjust_time("15m", "RB1805", datetime.now(), "20180403")
    # print(new_dt)
    # new_dt = tv.adjust_time("30m", "RB1805", datetime.now(), "20180403")
    # print(new_dt)
    # new_dt = tv.adjust_time("60m", "RB1805", datetime.now(), "20180403")
    # print(new_dt)
    # new_dt = tv.adjust_time("1d", "RB1805", datetime.now(), "20180403")
    # print(new_dt)
