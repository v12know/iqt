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


class BaseDataSource:
    def __init__(self):

        # self._instruments = InstrumentStore(_p('instruments.pk'))
        # self._trading_dates = TradingDatesStore(_p('trading_dates.bcolz'))
        pass

    def history_bars(self, instrument, bar_count, frequency, fields=None, dt=None,
                     skip_suspended=True, include_now=False,
                     adjust_type='pre', adjust_orig=None):
        raise NotImplementedError

