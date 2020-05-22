#!/usr/bin/python
# -*- coding: utf-8 -*-

'''一个简单的SINA数据客户端，主要使用requests开发'''

from datetime import datetime, timedelta
# import execjs
import re
import json
from urllib import request
from urllib.error import URLError, HTTPError

from pyiqt.data.base_data_source import BaseDataSource
from pyiqt.model.bar import Bar
from pyiqt.model.tick import Tick

if __name__ == '__main__':
    # import os
    # import sys
    from pyiqt.util.logger import LoggerFactory
    # base_path = os.path.dirname(os.path.realpath(sys.argv[0]))
    # log_dir = base_path
    logger = LoggerFactory().create_logger(log_dir="../../pylog")
else:
    from pyiqt.util.logger import logger


class SinaDataSource(BaseDataSource):
    def __init__(self):
        self.headers = {}

        self.p = re.compile("^(IF|IC|IH|T|TF)\d{4}$", re.IGNORECASE)
        self.p_tl = re.compile("^.*\((.+)\).*$")

    def _get(self, url):
        req = request.Request(url, headers=self.headers, method="GET")
        try:
            rsp_data = request.urlopen(req).read().decode('gbk')
        except HTTPError as e:
            logger.exception("HTTPError")
            raise e
        except URLError as e:
            logger.exception("URLError")
            raise e
        return rsp_data

    def get_time_line(self, order_book_id):
        # 从sina加载最新的M1数据(只有收盘价)
        order_book_id = order_book_id.upper()
        url = 'http://stock2.finance.sina.com.cn/futures/api/jsonp.php/var%20t1nf_{0}=/InnerFuturesNewService.getMinLine?symbol={0}'.format(order_book_id)
        logger.debug(u'从sina下载{0}的分时数据{1}'.format(order_book_id, url))
        rsp_data = self._get(url)
        results = self.p_tl.search(rsp_data)

        responses = json.loads(results.group(1))

        ticks = []
        for item in responses:
            tick = Tick()
            tick.order_book_id = order_book_id
            if len(item) >= 7:
                date_value = item[6]
                prev_close = float(item[5])
            time_value = item[0]
            tick.datetime = datetime.strptime(date_value + time_value, '%Y-%m-%d%H:%M') + timedelta(minutes=1)
            tick.last = float(item[1])
            tick.settlement = float(item[2])
            tick.volume = int(item[3])
            tick.open_interest = int(item[4])
            tick.prev_close = prev_close
            ticks.append(tick)

        if len(ticks) > 0:
            logger.debug(u'从sina读取了{0}条分时数据'.format(len(ticks)))
        else:
            logger.debug(u'从sina读取分时数据失败')
        return ticks

    def history_bars(self, instrument, bar_count, frequency, fields=None, dt=None,
                     skip_suspended=True, include_now=False,
                     adjust_type='pre', adjust_orig=None):

        if frequency in ['5m', '15m', '30m', '60m']:
            return self._get_minute_bars(instrument.order_book_id, int(frequency[:-1]), fields)[-bar_count:]
        elif frequency == '1d':
            return self._get_daily_bars(instrument.order_book_id, fields)[-bar_count:]
        else:
            raise NotImplementedError("wrong frequency parameter, should be in [5m, 15m, 30m, 60m, 1d]")

    def _get_daily_bars(self, order_book_id, fields):
        if self.p.search(order_book_id):
            url = "http://stock2.finance.sina.com.cn/futures/api/json.php/CffexFuturesService.getCffexFuturesDailyKLine?symbol={0}".format(order_book_id)
        else:
            url = "http://stock2.finance.sina.com.cn/futures/api/json.php/IndexService.getInnerFuturesDailyKLine?symbol={0}".format(order_book_id)
        bars = []
        logger.debug(u'从sina下载{0}的日线数据 {1}'.format(order_book_id, url))
        responses = json.loads(self._get(url))
        # responses = execjs.eval(self.session.get(url).content.decode('gbk'))
        for item in responses:
            # bar的close time
            sina_dt = datetime.strptime(item[0], '%Y-%m-%d')
            bar = Bar()
            bar.datetime = sina_dt
            bar.order_book_id = order_book_id
            # bar.date = bar.datetime.strftime('%Y%m%d')
            # bar.tradingDay = bar.date       # todo: 需要修改，晚上21点后，修改为next workingday
            bar.open = float(item[1])

            bar.high = float(item[2])

            bar.low = float(item[3])

            bar.close = float(item[4])

            bar.volume = int(item[5])
            bars.append(bar)
        if len(bars) > 0:
            logger.debug(u'从sina读取了{0}条日线数据'.format(len(bars)))
        else:
            logger.debug(u'从sina读取日线数据失败')

        return bars

    def _get_minute_bars(self, order_book_id, minute, fields):
        """# 从sina加载最新的M5,M15,M30,M60数据"""
        if self.p.search(order_book_id):
            url = "http://stock2.finance.sina.com.cn/futures/api/json.php/CffexFuturesService.getCffexFuturesMiniKLine{0}m?symbol={1}".format(minute, order_book_id)
        else:
            url = 'http://stock2.finance.sina.com.cn/futures/api/json.php/InnerFuturesService.getInnerFutures{0}MinKLine?symbol={1}'.format(minute, order_book_id)
        bars = []
        logger.debug(u'从sina下载{0}的{1}分钟数据 {2}'.format(order_book_id, minute, url))
        responses = json.loads(self._get(url))
        # responses = execjs.eval(self.session.get(url).content.decode('gbk'))
        for item in responses:
            # bar的close time
            sina_dt = datetime.strptime(item[0], '%Y-%m-%d %H:%M:%S')
            if minute in {5, 15} and sina_dt.hour == 10 and sina_dt.minute == 30:
                # 这个是sina的bug，它把10:15 ~10:30也包含进来了
                continue
            bar = Bar()
            bar.datetime = sina_dt
            bar.order_book_id = order_book_id
            # bar.date = bar.datetime.strftime('%Y%m%d')
            # bar.tradingDay = bar.date       # todo: 需要修改，晚上21点后，修改为next workingday
            bar.open = float(item[1])

            bar.high = float(item[2])

            bar.low = float(item[3])

            bar.close = float(item[4])

            bar.volume = int(item[5])
            bars.append(bar)
        if len(bars) > 0:
            logger.debug(u'从sina读取了{0}条{1}分钟数据'.format(len(bars), minute))
        else:
            logger.debug(u'从sina读取{0}分钟数据失败'.format(minute))

        return bars


if __name__ == '__main__':
    # SinaClient().get_bars('1d', 'zc1806')
    SinaDataSource().get_time_line('zc1805')
