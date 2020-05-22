#!/usr/bin/python
# -*- coding: utf-8 -*-

from pyiqt.util.decorator import singleton


# from ..util.logger import log
# log.info("subject-cccc......adfdfdf1233333333333-----------------")
@singleton
class TickSubject(object):
    def __init__(self):
        self.observerMap = {}

    def notify(self, tick):
        # print("observerMap = %s" % self.observerMap)
        if tick.symbol in self.observerMap.keys():
            observerList = self.observerMap[tick.symbol]
            for observer in observerList:
                # print("tick = %s, observer = %s" % (tick, self.observerMap))
                observer.update(tick=tick)

    def attach(self, symbol, observer):
        print('ticksubject symbol = %s, observer = %s' % (symbol, observer))
        if symbol in self.observerMap.keys():
            observerList = self.observerMap[symbol]
            print("old observerMap")
        else:
            observerList = []
            self.observerMap[symbol] = observerList
            print("new observerMap")
        observerList.append(observer)
        print("observerList = %s, observerMap = %s" % (observerList, self.observerMap))

    def detach(self, symbol, observer):
        # exit(1)
        if symbol in self.observerMap.keys():
            observerList = self.observerMap[symbol]
            observerList.remove(observer)


@singleton
class BarSubject(object):
    def __init__(self):
        self.observerMap = {}

    def notify(self, observerId, bar, barList):
        observerList = self.observerMap[observerId]
        for observer in observerList:
            observer.update(bar, barList)

    def attach(self, observerId, observer):
        if observerId in self.observerMap.keys():
            observerList = self.observerMap[observerId]
        else:
            observerList = []
            self.observerMap[observerId] = observerList
        observerList.append(observer)

    def detach(self, observerId, observer):
        if observerId in self.observerMap.keys():
            observerList = self.observerMap[observerId]
            observerList.remove(observer)
