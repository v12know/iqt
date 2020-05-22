#!/usr/bin/python
# -*- coding: utf-8 -*-
# import py.portal_iqt

from pyiqt.data.sina_data_source import SinaDataSource

SinaDataSource().getMinBars('cu1706', 5, None)
# exit(0)

import os
import signal


def term(sig_num, addtion):
    print('current pid is %s, group id is %s' % (os.getpid(), os.getpgrp()))
    os.killpg(os.getpgid(os.getpid()), signal.SIGKILL)


signal.signal(signal.SIGTERM, term)
signal.signal(signal.SIGINT, term)

# from py import log
from pyiqt.util.logger import log

log.info("adfdfdf1233333333333-----------------")
import time

if __name__ == '__main__':
    time.sleep(1000)
    # from py.util.proc_term import ProcTerm
    # TickConsumer().start()
    # ProcTerm().join()
    # exit(0)

    # tick = py.portal_iqt.Tick()
    # tick.tradingDay = "20170424"
    # tick.instrumentId = "cu1705"
    # tick.lastPrice = 41800
    # tick.volume = 12
    # tick.datetime = "2017042611:28:110"
    # print(tick.datetime)
    # # bar = Bar('30M', tick = tick)
    # # print(bar.datetime)
    # # # tick.rcvTime = datetime.datetime.today()
    # # print(isinstance(tick.rcvTime, datetime.datetime))
    # barBuilder = BarBuilderFactory().create(instrumentId = 'cu1705', frequency = '15m')
    # TickSubject().notify(tick)
    # def tttt(tick1):
    # print("tick================" + repr(tick1))
    # py.portal_iqt.regOnRtnTick(tttt)
    # TickConsumer().putTick(tick)
