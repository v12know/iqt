#!/usr/bin/python
# -*- coding: utf-8 -*-
from multiprocessing import Process, Queue
from pyiqt.util.decorator import singleton
from pyiqt.util.logger import LoggerFactory


log = LoggerFactory().get_logger()


# from py.portal_iqt import Tick

@singleton
class TickConsumer:
    def __init__(self):
        self.queue = Queue()
        # self.process = Process(target = self.run, args = (self.queue,))
        self.process = Process(target=self.run)
        # self.process.daemon = True
        self.runFlag = True

    def start(self):
        self.process.start()

    def join(self):
        self.process.join()

    # @staticmethod
    def run(self):
        from pyiqt.model.bar import BarBuilderFactory
        BarBuilderFactory().create(symbol='cu1705', frequency='5m')
        BarBuilderFactory().create(symbol='cu1706', frequency='5m')

        while self.runFlag:
            self.consume()
            # print("TickConsumer.run....")

    def consume(self):
        tick = self.queue.get()

        # import pickle
        # tick = pickle.loads(tick)
        # print(tick.tradingDay)
        # print(str(tick))
        from pyiqt.trade.subject import TickSubject
        # log.debug("*********************TickSuject id = %s" % TickSubject())
        TickSubject().notify(tick)

    def putTick(self, tick):
        self.queue.put_nowait(tick)

