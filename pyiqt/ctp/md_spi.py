#!/usr/bin/python
# -*- coding: utf-8 -*-
from pyiqt.ctp import portal_ctp
from pyiqt.util.decorator import singleton
from pyiqt.trade.consumer import TickConsumer


# def onRtnTick(tick):
# print("testmd===========" + str(tick))
# portal_iqt.regOnRtnTick(onRtnTick)

@singleton
class MdSpiCbk:
    def __init__(self):
        # portal_ctp.regOnRtnTick(self.onRtnTick)
        pass

    def onRtnTick(self, tick):
        TickConsumer().putTick(tick)
        pass


MdSpiCbk()
