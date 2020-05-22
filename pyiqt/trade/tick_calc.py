#!/usr/bin/python
# -*- coding: utf-8 -*-

from pyiqt.trade.subject import TickSubject
from pyiqt.trade.observer import BarObserver


class TickCalc(object):
    def calc(self, tick):
        pass


class SingleTickCalc(TickCalc):
    def __init__(self, instrumentId, tickObserver):
        print('init SingleTickCalc ' + instrumentId + "=========================")
        print("TickSuject id = %s" % TickSubject())
        TickSubject().attach(instrumentId, tickObserver)

    def calc(self, tick):
        return [tick]


class PairTickCalc(TickCalc):
    def __init__(self, pairInstrumentId, tickObserver):
        self.mainMdList = []
        self.hedgeMdList = []
        self.mainInstrumentId = ''
        self.hedgeInstrumentId = ''
        TickSubject().attach(self.mainInstrumentId, tickObserver)
        TickSubject().attach(self.hedgeInstrumentId, tickObserver)

    def calc(self, tick):
        if tick["instrumentId"] == self.mainInsturmentId:
            self.mainMdList.append(tick)
            self.__calc(tick, True, self.mainMdList, self.hedgeMdList)
        else:
            self.hedgeMdList.append(tick)
            self.__calc(tick, False, self.hedgeMdList, self.mainMdList)
        return tick

    def __calcPair(self, mainFlag, md1, md2):
        if mainFlag:
            mainMd = md1
            hedgeMd = md2
        else:
            mainMd = md2
            hedgeMd = md1
        md = Tick()
        # TODO +-*/
        md.datetime = max(mainMd.datetime, hedgeMd.datetime)
        md.volume = min(mainMd.volume, hedgeMd.volume)
        md.lastPrice = mainMd.lastPrice - hedgeMd.lastPrice
        return md

    def __calc(self, tick, mainFlag, list1, list2):
        mdList = []
        if len(list2) == 0:
            return mdList
        if list1[-1].datetime <= list2[-1].datetime:
            i = len(list1) - 1
            j = len(list2) - 1
            delPos1 = -1
            delPos2 = -1
            delFlag = False
            while True:
                if i < 0 or j < 0:
                    break
                if list1[i].datetime < list2[j].datetime:
                    if delFlag == False:
                        j -= 1
                        continue
                    else:
                        mdList.insert(0, self.__calcPair(mainFlag, list1[i], list2[j]))
                        j -= 1
                else:
                    mdList.insert(0, self.__calcPair(mainFlag, list1[i], list2[j]))
                    if delFlag == False:
                        delFlag = True
                        delPos1 = i
                        delPos2 = j
                    if list1[i].datetime == list2[j].datetime:
                        i -= 1
                        j -= 1
                    if list1[i].datetime > list2[j].datetime:
                        i -= 1
            if delFlag == True:
                del list1[0:delPos1 + 1]
                del list2[0:delPos2 + 1]
        return mdList
