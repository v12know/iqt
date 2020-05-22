# -*- coding: utf-8 -*-
import re

from pyiqt.stgy import portal_stgy as stgy
from pyiqt.util.logger import LoggerFactory
from pyiqt.model.tick import Tick

import simplejson

CODE_INFO_TAB_NAME = "CODE_INFO"
MAIN_CONTRACT_SUFFIX = "88"
INDEX_CONTRACT_SUFFIX = "99"
# frequencies = ['tick', '1m', '5m', '30m', '60m', '1d']
# frequencies = ['tick', '1m', '1d']


# class CollectConfig(object):
#     def __init__(self):
#         self.stgy_id = None
#         self.collect_pattern = None


class CollectStgy(stgy.AbstractStgy):
    def __init__(self, config):
        super(CollectStgy, self).__init__()
        self.config = config
        self.p = re.compile("^(\D+)\d{4}$")
        self.logger = LoggerFactory().create_logger(self.config["stgy_id"], level=self.config["log_level"])

        self.symbol_list_map_subscribed = {}
        self.symbol_list_map_all = {}

        if not self.config["just_log"]:
            self.thread_num = self.config["thread_num"]
            from kafka import KafkaProducer
            import happybase
            self.producer = KafkaProducer(bootstrap_servers='hadoop201:9092,hadoop203:9092,hadoop204:9092',)
                                          #value_serializer=lambda v: json.dumps(v).encode('utf-8'))
            self.pool = happybase.ConnectionPool(size=self.thread_num, host='hadoop201',
                                                 autoconnect=True)  # , transport='framed', protocol='binary')
            self.families = {
                'i': dict(max_versions=1),
            }

    def _worker(self, start, step, list_list):
        with self.pool.connection() as connection:
            connection.open()
            table_names = set(connection.tables())
            new_main_table_names = set()
            new_index_table_names = set()
            for i in range(start, len(list_list), step):
                for order_book_id in list_list[i]:
                    matcher = re.match(self.p, order_book_id)
                    if not matcher:
                        continue
                    main_contract = matcher.group(1) + MAIN_CONTRACT_SUFFIX
                    index_contract = matcher.group(1) + INDEX_CONTRACT_SUFFIX
                    for frequency in self.config["frequencies"]:
                        frequency = frequency.strip()
                        table_name = "f_{}:{}".format(frequency, order_book_id)
                        main_table_name = "f_{}:{}".format(frequency, main_contract)
                        index_table_name = "f_{}:{}".format(frequency, index_contract)
                        if table_name.encode('utf-8') not in table_names:
                            self.logger.warn("create table: {}".format(table_name))
                            connection.create_table(table_name, self.families)
                        if frequency != "tick" and main_table_name.encode("utf-8") not in table_names \
                                and main_table_name not in new_main_table_names:
                            self.logger.warn("create table: {}".format(main_table_name))
                            connection.create_table(main_table_name, self.families)
                            new_main_table_names.add(main_table_name)
                        if frequency != "tick" and index_table_name.encode("utf-8") not in table_names \
                                and index_table_name not in new_index_table_names:
                            self.logger.warn("create table: {}".format(index_table_name))
                            connection.create_table(index_table_name, self.families)
                            new_index_table_names.add(index_table_name)

    def _check_tables(self, context):
        with self.pool.connection() as connection:
            connection.open()
            table_names = set(connection.tables())

            if CODE_INFO_TAB_NAME.encode("utf-8") not in table_names:
                connection.create_table(CODE_INFO_TAB_NAME, self.families)
            # self.logger.debug("tables:\n{}".format(table_names))

        import threading
        # self._check_tables(context)
        list_list = list(self.symbol_list_map_subscribed.values())
        for i in range(self.thread_num):
            threading.Thread(target=self._worker, args=(i, self.thread_num, list_list), name='thread-' + str(i)) \
                .start()

    def _build_symbol_list_map(self, universe, symbol_list_map):
        for order_book_id in universe:
            matcher = re.match(self.p, order_book_id)
            if not matcher:
                continue
            underlying_symbol = matcher.group(1)
            if underlying_symbol not in symbol_list_map:
                symbol_list_map[underlying_symbol] = [order_book_id]
            else:
                symbol_list_map[underlying_symbol].append(order_book_id)

    def init(self, context):

        if not self.config["just_log"]:
            self._build_symbol_list_map(context.universe, self.symbol_list_map_subscribed)

            from pyiqt import portal_iqt
            instruments = portal_iqt.Env.instruments
            self._build_symbol_list_map(instruments.keys(), self.symbol_list_map_all)

            self._check_tables(context)

        self.logger.warn("collect init")
        return True

    def handle_tick(self, context, tick):
        # self.logger.debug("collect handle_tick")
        tick.fix_tick()
        # self.logger.debug(str(tick.trading_date))

        new_tick_json = simplejson.dumps(Tick(tick).__dict__)
        self.logger.info(new_tick_json)
        # self.producer.send("test", json.dumps(new_tick).encode("utf-8"))
        if not self.config["just_log"]:
            self.producer.send(self.config["topic"], key=re.match(self.p, tick.order_book_id).group(1).encode('utf-8'),
                               value=new_tick_json.encode('utf-8'))

    def before_trading(self, context):
        self.logger.debug("before_trading")
        if not self.config["just_log"]:
            with self.pool.connection() as connection:
                connection.open()
                map_json = simplejson.dumps(self.symbol_list_map_all)
                self.logger.info("map_json: " + map_json)
                table = connection.table(CODE_INFO_TAB_NAME)
                table.put("UNDERLYING_SYMBOL_LIST_MAP", {"i:value": map_json})

    def after_trading(self, context):
        self.logger.debug("after_trading")
