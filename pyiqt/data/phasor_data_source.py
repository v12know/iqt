import pandas as pd
from urllib import request, parse
from urllib.error import URLError, HTTPError
import json
from datetime import datetime
from datetime import timedelta
import time

from pyiqt.util.decorator import singleton


@singleton
class PhasorDataSource:
    def __init__(self):
        self._base_url = "http://hadoop201:5000/api/v1/"
        self._bar_url = "{}{}".format(self._base_url, "history_bars")
        self._holiday_url = "{}{}".format(self._base_url, "is_holiday")
        self._trade_cal_url = "{}{}".format(self._base_url, "trade_cal")
        self._headers = {}

    def history_bars(self, order_book_id, bar_count, frequency, fields, dt=None,
                     include_now=False, adjust_type='pre', adjust_orig=None):
        if frequency not in ('1m', '5m', '15m', '30m', '60m', '1d', 'tick'):
            raise NotImplementedError

        fields_str = fields.replace(" ", "") if isinstance(fields, str) else ",".join(fields)
        fields_str = fields_str if fields_str.find("datetime") >= 0 else fields_str + ",datetime"
        if fields_str.count(",") == 1:
            import re
            p = re.compile(",?datetime,?")
            single_field = re.sub(p, '', fields_str)
        else:
            single_field = None
        values = {"order_book_id": order_book_id, "frequency": frequency, "include_now": include_now,
                  "bar_count": bar_count, "fields": fields_str}
        data = parse.urlencode(values).encode("utf-8")
        req = request.Request(self._bar_url, headers=self._headers, data=data, method="GET")
        retry_times = 0
        while True:
            try:
                rsp_data = request.urlopen(req).read()
            except HTTPError as e:
                print(e.reason)
                retry_times += 1
                if retry_times >= 5:
                    raise e
                time.sleep(1)
            except URLError as e:
                print(e.reason)
                retry_times += 1
                if retry_times >= 5:
                    raise e
                time.sleep(1)
            else:
                break
        # json_data = json.loads(rsp_data.decode("gbk"))
        # print(json_data)
        df = pd.read_json(rsp_data, orient="records", encoding="gbk").set_index("datetime")
        # df = pd.read_json(json_data, orient="index", typ="frame")
        # df.index.names = ['datetime']
        # if fields_str.find(",") < 0:
        #     df = df[fields_str]
        if single_field:
            df = df[single_field]
        # print(df)
        # print(type(df))

        return df

    def is_holiday(self, dt):
        values = {"dt": dt}
        data = parse.urlencode(values).encode("utf-8")
        req = request.Request(self._holiday_url, headers=self._headers, data=data, method="GET")
        try:
            rsp_data = request.urlopen(req).read()
        except HTTPError as e:
            print(e.reason)
        except URLError as e:
            print(e.reason)
        json_data = json.loads(rsp_data.decode("gbk"))
        return json_data["res"]

    def trade_cal(self, begin_dt, end_dt=None, days=7):
        if end_dt is None:
            end_dt = (datetime.strptime(begin_dt, "%Y-%m-%d") + timedelta(days=days - 1)).strftime("%Y-%m-%d")
        values = {"begin_dt": begin_dt, "end_dt": end_dt}
        data = parse.urlencode(values).encode("utf-8")
        req = request.Request(self._trade_cal_url, headers=self._headers, data=data, method="GET")
        try:
            rsp_data = request.urlopen(req).read()
        except HTTPError as e:
            print(e.reason)
        except URLError as e:
            print(e.reason)
        return json.loads(rsp_data.decode("gbk"))
