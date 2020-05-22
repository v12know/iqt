# -*- coding: utf-8 -*-
#
# Copyright 2017 Ricequant, Inc
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from pyiqt.environment import Environment


class DataProxy:
    def __init__(self, data_source):
        self._data_source = data_source

    def __getattr__(self, item):
        return getattr(self._data_source, item)

    def history(self, order_book_id, bar_count, frequency, field, dt):
        data = self.history_bars(order_book_id, bar_count, frequency,
                                 ['datetime', field], dt, skip_suspended=False, adjust_orig=dt)
        if data is None:
            return None
        return pd.Series(data[field], index=[convert_int_to_datetime(t) for t in data['datetime']])

    def fast_history(self, order_book_id, bar_count, frequency, field, dt):
        return self.history_bars(order_book_id, bar_count, frequency, field, dt, skip_suspended=False,
                                 adjust_type='pre', adjust_orig=dt)

    def history_bars(self, order_book_id, bar_count, frequency, field=None, dt=None,
                     skip_suspended=True, include_now=False,
                     adjust_type='pre', adjust_orig=None):
        instrument = Environment().get_instrument(order_book_id)
        if adjust_orig is None:
            adjust_orig = dt
        return self._data_source.history_bars(instrument, bar_count, frequency, field, dt,
                                              skip_suspended=skip_suspended, include_now=include_now,
                                              adjust_type=adjust_type, adjust_orig=adjust_orig)

