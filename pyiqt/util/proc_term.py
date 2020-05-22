#!/usr/bin/python
# -*- coding: utf-8 -*-

from functools import partial
from pyiqt.util.decorator import singleton
import signal
import os


@singleton
class ProcTerm():
    def __init__(self):
        self.processes = []
        handler = partial(self.term, self.processes)
        # signal.signal(signal.SIGTERM, handler)

    def addProcess(self, process):
        self.processes.append(process)

    @staticmethod
    def term(processes, sig_num, frame):
        print('terminate process %d' % os.getpid())
        try:
            print('the processes is %s' % processes)
            for p in processes:
                print('process %d terminate' % p.pid)
                p.terminate()
        except Exception as e:
            print(str(e))

    def join(self):
        try:
            for p in self.processes:
                p.join()
        except Exception as e:
            print(str(e))
