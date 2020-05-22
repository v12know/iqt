#!/usr/bin/python
# -*- coding: utf-8 -*-
import sys
import os
import re
from logging.handlers import TimedRotatingFileHandler
from logging import StreamHandler
import logging
import logging.config
from pyiqt.util.decorator import singleton
import datetime
import time


@singleton
class LoggerFactory:
    _logger_map = {}

    def __init__(self):
        self._log_dir = os.path.join(os.path.dirname(os.path.realpath(sys.argv[0])), 'pylog')
        self._formatter = logging.Formatter(
            '%(asctime)s %(levelname)s [%(filename)s:%(lineno)d]: %(message)s')

    def create_logger(self, name="default", level=logging.INFO, console_flag=True, log_dir=None):
        print("create_logger: name={}".format(name))
        log = logging.getLogger(name)
        log.setLevel(level)
        if not log_dir:
            log_dir = self._log_dir

        if not os.path.exists(log_dir):
            os.makedirs(log_dir)
        file_handler = TimedRotatingFileHandler(filename=os.path.join(log_dir, name + '.log'), when="H",
                                                interval=1, backupCount=24)
        file_handler.suffix = "%Y-%m-%d_%H-%M.log"
        file_handler.extMatch = re.compile(r"^\d{4}-\d{2}-\d{2}_\d{2}-\d{2}.log$")
        # file_handler.setLevel(level)
        file_handler.setFormatter(self._formatter)
        log.addHandler(file_handler)

        if console_flag:
            console_handler = StreamHandler(sys.stdout)
            # console_handler.setLevel(level)
            console_handler.setFormatter(self._formatter)
            log.addHandler(console_handler)

        self._logger_map[name] = log

        # if not self._trunc_flag:
        #     self.trunc_log_dir(self._log_dir, 3)
        return log

    def get_logger(self, name="default"):
        print("get_logger: name={}".format(name))
        logger = self._logger_map[name]

        return logger

    def trunc_log_dir(self, log_dir=None, keep_days=3):
        if not log_dir:
            log_dir = self._log_dir
        # 取得当前时间
        today = datetime.datetime.now()
        # 取得N日前日期
        keep_days_ago = today - datetime.timedelta(days=keep_days)
        # 将时间转成timestamps
        keep_days_ago_timestamps = time.mktime(keep_days_ago.timetuple())
        print("进入目录：%s" % log_dir)
        print("开始删除%d天前面的日志。。。" % keep_days)

        for file in os.listdir(log_dir):
            # 忽略.开头文件
            # if i.startswith('.'):
            #     continue
            # 忽略目录
            if os.path.isdir(os.path.join(log_dir, file)):
                continue
            # 获取文件的modify时间，并转化成timestamp格式
            file_timestamp = os.path.getmtime(os.path.join(log_dir, file))
            # print(file_timestamp)

            # 比较文件modify时间和3日前时间，取出小于等于3日前日期的文件
            if float(file_timestamp) <= float(keep_days_ago_timestamps):
                print("正在删除：%s" % os.path.join(log_dir, file))
                os.remove(os.path.join(log_dir, file))

        print("删除日志完毕！")


# logger = LoggerFactory().create_logger()