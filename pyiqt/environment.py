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
from pyiqt.events import EventBus
from pyiqt.util.decorator import singleton
import pyiqt.portal_iqt as iqt


@singleton
class Environment(object):

    def __init__(self):
        self._env = iqt.Env.instance()
        self.data_proxy = None
        self.data_source = None
        self.event_bus = EventBus()
        self.calendar_dt = None

    def set_data_proxy(self, data_proxy):
        self.data_proxy = data_proxy

    def set_data_source(self, data_source):
        self.data_source = data_source

    @property
    def universe(self):
        return self._env.universe

    @universe.setter
    def universe(self, universe):
        self._env.universe = universe

    def get_all_instruments(self):
        return self._env.instruments

    def get_instrument(self, order_book_id):
        return self._env.instruments[order_book_id]

    @property
    def trading_date(self):
        return self._env.trading_date

