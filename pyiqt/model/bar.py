#!/usr/bin/python
# -*- coding: utf-8 -*-

import datetime

from pyiqt.util.decorator import singleton
# from .observer.subject import TickSubject
from pyiqt.trade.subject import BarSubject
from pyiqt.trade.observer import TickObserver
from pyiqt.trade.observer import BarObserver
from pyiqt.trade.tick_calc import SingleTickCalc
# from py.trade.tick_calc import PairTickCalc
# from pyspark import SparkContext, SparkConf
#
# conf = SparkConf().setAppName("pyspark_test").setMaster('local[*]')
# sc = SparkContext(conf=conf)
# from py import log
# from py.util.logger import log
# log.info("bar-cccc......adfdfdf1233333333333-----------------")
class Bar(object):
    def __init__(self, frequency='', tick=None, bar=None):
        if tick is not None:
            self._init_by_tick(frequency, tick)
        elif bar is not None:
            self._init_by_bar(frequency, tick)
        else:
            self._init_default()

    def _init_default(self):
        self.order_book_id = ''
        self.datetime = None  # python的datetime对象

        self.open = 0
        self.high = 0
        self.low = 0
        self.close = 0
        self.volume = 0

        self.limit_up = 0
        self.limit_down = 0
        self.open_interest = 0
        self.total_turnover = 0
        self.trading_date = None

    def _init_by_tick(self, frequency, tick):
        self.order_book_id = tick.symbol
        dt = datetime.datetime.strptime(tick.datetime, '%Y%m%d%H:%M:%S%f')
        self.datetime = TruncDateTime().trunc(dt, frequency)  # python的datetime对象

        self.open = tick.lastPrice
        self.high = tick.lastPrice
        self.low = tick.lastPrice
        self.close = tick.lastPrice
        self.volume = tick.volume

        self.limit_up = tick.limitUp
        self.limit_down = tick.limitDown
        self.open_interest = tick.openInterest
        self.total_turnover = tick.turnover
        self.trading_date = tick.tradeDate

    def _init_by_bar(self, frequency, bar):
        self.order_book_id = bar.symbol
        self.datetime = TruncDateTime().trunc(bar.datetime, frequency)  # python的datetime对象

        self.open = bar.open
        self.high = bar.high
        self.low = bar.low
        self.close = bar.close
        self.volume = bar.volume

        self.limit_up = bar.limit_up
        self.limit_down = bar.limit_down
        self.open_interest = bar.open_interest
        self.total_turnover = bar.total_turnover
        self.trading_date = bar.trading_date

    def update_by_tick(self, frequency, tick):
        dt = datetime.datetime.strptime(tick.datetime, '%Y%m%d%H:%M:%S%f')
        dt = TruncDateTime().trunc(dt, frequency)
        if self.datetime == dt:
            if self.high < tick.lastPrice:
                self.high = tick.lastPrice
            if self.low > tick.lastPrice:
                self.low = tick.lastPrice
            self.close = tick.lastPrice
            self.volume += tick.volume
            return True
        return False

    def update_by_bar(self, frequency, bar):
        dt = TruncDateTime().trunc(bar.datetime, frequency)  # python的datetime对象
        if self.datetime == dt:
            if self.high < bar.high:
                self.high = bar.high
            if self.low > bar.low:
                self.low = bar.low
            self.close = bar.close
            self.volume += bar.volume
            return True
        return False


@singleton
class TruncDateTime:
    def __init__(self):
        self.funcs = {'m': self.__minute, 'h': self.__hour, 'd': self.__day, 'w': self.__week, 'M': self.__month, 'y': self.__year}

    def trunc(self, dt, frequency):
        return self.funcs[frequency[-1]](dt, int(frequency[0:-1]))

    @staticmethod
    def __minute(dt, num):
        return datetime.datetime(dt.year, dt.month, dt.day, dt.hour, int(dt.minute / num) * num, 0)

    @staticmethod
    def __hour(dt, num):
        return datetime.datetime(dt.year, dt.month, dt.day, int(dt.hour / num) * num, 0, 0)

    @staticmethod
    def __day(dt, num):
        return datetime.datetime(dt.year, dt.month, int(dt.day / num) * num, 0, 0, 0)

    @staticmethod
    def __week(dt, num):
        '''只支持1周'''
        '''firstDt = datetime.datetime(dt.year, 1, 1, 0, 0, 0)
        firstDtCount = datetime.timedelta(firstDt.isoweekday())
        firstWeekday = firstDt - firstDtCount
        dtCount = datetime.timedelta(days = dt.isoweekday())
        weekday = dt - dtCount
        diffDays = weekday - firstWeekday
        diffDays = int(diffDays.days / (7 * num)) * (7 * num)
        newDt = firstWeekday + datetime.timedelta(days = diffDays)
        return datetime.datetime(newDt.year, newDt.month, newDt.day, 0, 0, 0)'''
        dtCount = datetime.timedelta(days = dt.isoweekday())
        weekday = dt - dtCount
        return datetime.datetime(weekday.year, weekday.month, weekday.day, 0, 0, 0)


    @staticmethod
    def __month(dt, num):
        return datetime.datetime(dt.year, int(dt.month / num) * num, 1, 0, 0, 0)

    @staticmethod
    def __year(dt, num):
        return datetime.datetime(int(dt.year / num) * num, 1, 1, 0, 0, 0)



# print "isoweekday=" + str(datetime.datetime.now().isoweekday())
# print TruncDateTime().trunc(datetime.datetime.now(), '13w')
# print TruncDateTime().trunc(datetime.datetime.now(), '1m')
# print TruncDateTime().trunc(datetime.datetime.now(), '15M')
# print TruncDateTime().trunc(datetime.datetime.now(), '3M')
# print TruncDateTime().trunc(datetime.datetime.now(), '1d')
# print TruncDateTime().trunc(datetime.datetime.now(), '10d')
# print TruncDateTime().trunc(datetime.datetime.now(), '3m')
# print TruncDateTime().trunc(datetime.datetime.now(), '1m')
# print TruncDateTime().trunc(datetime.datetime.now(), '1y')


class BarM1Builder(object):
    def __init__(self, *args, **kwargs):
        self.symbol = kwargs["symbol"]
        self.frequency = kwargs["frequency"]
        self.id = self.symbol + ":" + self.frequency
        self.barList = []
        print(self.symbol + ":1m")
        self.observer = TickObserver(self)
        if self.symbol.find("+-*/") >= 0:
            self.calculator = PairTickCalc(self.symbol, self.observer)
        else:
            self.calculator = SingleTickCalc(self.symbol, self.observer)

    def appendTick(self, tick):
        # print("appendTick: tick=%s" % tick)
        # print("symbol = %s, barList size = %d" % (tick.symbol, len(self.barList)))
        tickList = self.calculator.calc(tick)
        if tickList == None:
            return
        for tick in tickList:
            if len(self.barList) == 0:
                bar = Bar(self.frequency, tick=tick)
                self.barList.append(bar)
            else:
                lastBar = self.barList[-1]
                # print("lastBar datetime = %s" % str(lastBar.datetime))
                if not lastBar.update_by_tick(self.frequency, tick):
                    newBar = Bar(self.frequency, tick=tick)
                    self.barList.append(newBar)
                    BarSubject().notify(str(self), bar=self.barList[-1], barList=self.barList)
                    # print(self.barList)

    def __str__(self):
        return self.id


class BarBuilder(object):
    def __init__(self, *args, **kwargs):
        self.symbol = kwargs["symbol"]
        self.frequency = kwargs["frequency"]
        self.id = self.symbol + ":" + self.frequency
        self.barList = []
        kwargs["barBuilderFactory"].create(symbol=self.symbol, frequency='1m')
        self.observer = BarObserver(self)
        BarSubject().attach(self.symbol + ":1m", self.observer)

    def appendBar(self, bar, barList):
        print("symbol = %s, barList size = %d" % (bar.symbol, len(self.barList)))
        if len(self.barList) == 0:
            newBar = Bar(self.frequency, bar=bar)
            self.barList.append(newBar)
        else:
            lastBar = self.barList[-1]
            if not lastBar.update_by_bar(self.frequency, bar):
                newBar = Bar(self.frequency, bar=bar)
                self.barList.append(newBar)
                BarSubject().notify(str(self), bar=self.barList[-1], barList=self.barList)

    def __str__(self):
        return self.id


@singleton
class BarBuilderFactory(object):
    def __init__(self):
        self.barBuilderMap = {}

    def create(self, *args, **kwargs):
        barBuilderId = kwargs["symbol"] + ":" + kwargs["frequency"]
        if barBuilderId in self.barBuilderMap.keys():
            barBuilder = self.barBuilderMap[barBuilderId]
        else:
            kwargs["barBuilderFactory"] = self
            if kwargs["frequency"] == "1m":
                barBuilder = BarM1Builder(*args, **kwargs)
            else:
                barBuilder = BarBuilder(*args, **kwargs)
            self.barBuilderMap[barBuilderId] = barBuilder
        return barBuilder
