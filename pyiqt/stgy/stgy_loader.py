#!/usr/bin/python
# -*- coding: utf-8 -*-
from pyiqt.const import RUN_TYPE
from pyiqt.util.decorator import singleton
from multiprocessing import cpu_count
from pyiqt.stgy import portal_stgy
from pyiqt import portal_iqt
from pyiqt.util.logger import LoggerFactory

logger = LoggerFactory().get_logger()


@singleton
class StgyLoader:
    def __init__(self):
        self._stgy_executor_list = []

    @staticmethod
    def _parse_slots_pattern(slots_pattern):
        slots_list = slots_pattern.split(",")
        slots = []
        for item in slots_list:
            pair = item.split("*")
            if len(pair) == 1:
                slots.append(int(pair[0]))
            else:
                for i in range(int(pair[1])):
                    slots.append(int(pair[0]))
        return slots

    def _load_single_grid(self, config, stgy_executor):
        stgy_config = portal_stgy.SingleGridConfig()
        stgy_config.stgy_type = config["stgy_type"]
        stgy_config.stgy_id = config["stgy_id"]
        stgy_config.version = config["version"]
        stgy_config.universe = config["universe"]
        stgy_config.log_level = config["log_level"] if "log_level" in config else "info"
        print(stgy_config.universe)
        stgy_config.slots = self._parse_slots_pattern(config["slots_pattern"])
        stgy_config.open_spread = config["open_spread"]
        if "close_spread" in config:
            stgy_config.close_spread = config["close_spread"]
        else:
            stgy_config.close_spread = stgy_config.open_spread
        stgy_config.close_today = config["close_today"]
        context = portal_stgy.StgyContext()
        context.universe = set(config["universe"])
        logger.info("正在加载策略：{}".format(stgy_config.stgy_id))
        stgy_instance = portal_stgy.SingleGridStgy(stgy_config)
        stgy_instance.set_stgy_context(context)
        stgy_executor.append_stgy(stgy_instance)

    def _load_time_line(self, config, stgy_executor):
        stgy_config = portal_stgy.TimeLineConfig()
        stgy_config.stgy_type = config["stgy_type"]
        stgy_config.stgy_id = config["stgy_id"]
        stgy_config.version = config["version"]
        stgy_config.universe = config["universe"]
        stgy_config.log_level = config["log_level"] if "log_level" in config else "info"
        print(stgy_config.universe)
        stgy_config.spread = config["spread"]
        stgy_config.slot = config["slot"]
        stgy_config.close_today = config["close_today"]
        context = portal_stgy.StgyContext()
        context.universe = set(config["universe"])
        logger.info("正在加载策略：{}".format(stgy_config.stgy_id))
        stgy_instance = portal_stgy.TimeLineStgy(stgy_config)
        stgy_instance.set_stgy_context(context)
        stgy_executor.append_stgy(stgy_instance)

    def _load_collect(self, config, stgy_executor):
        from pyiqt.stgy.collect import collect_stgy

        # stgy_config = collect_stgy.CollectConfig()
        # stgy_config.stgy_type = config["stgy_type"]
        # stgy_config.stgy_id = config["stgy_id"]
        # stgy_config.version = config["version"]
        # stgy_config.collect_pattern = "^(" + config["collect_pattern"]\
        #                               + ")\\d*$" if "collect_pattern" in config else "^.+$"
        config["collect_pattern"] = "^(" + config["collect_pattern"]\
                                      + ")\\d*$" if "collect_pattern" in config else "^.+$"
        # stgy_config.topic = config["topic"]
        # stgy_config.frequencies = config["frequencies"]
        # stgy_config.just_log = config["just_log"]

        def parse_log_level(log_level):
            import logging
            return logging.getLevelName(log_level.upper())
        config["log_level"] = parse_log_level(config["log_level"] if "log_level" in config else "info")
        context = portal_stgy.StgyContext()
        instruments = portal_iqt.Env.instruments
        import re
        p = re.compile(config["collect_pattern"])  #
        p1 = re.compile("^\\D+\\d{4}$")
        universe = set()
        for k, v in instruments.items():
            if p.search(k) and p1.search(k):
                universe.add(k)
        context.universe = universe
        logger.info("正在加载策略：{}".format(config["stgy_id"]))
        stgy_instance = collect_stgy.CollectStgy(config)
        stgy_instance.set_stgy_context(context)
        stgy_executor.append_stgy(stgy_instance)

    def _load(self, stgy_conf, stgy_executor):
        stgy_type = stgy_conf["stgy_type"]
        if stgy_type == "single_grid":
            self._load_single_grid(stgy_conf, stgy_executor)
        if stgy_type == "time_line":
            self._load_time_line(stgy_conf, stgy_executor)
        if stgy_type == "collect":
            self._load_collect(stgy_conf, stgy_executor)

    '''@staticmethod
    def init_universe(config):
        stgys = config["stgys"]
        universe = set()
        # if len(stgys) == 0:
        #     return
        if config["run_type"] == RUN_TYPE.COLLECT:
            instruments = portal_iqt.Env.get_instruments()
            for k, v in instruments.items():
                universe.add(k)
        else:
            for stgy in stgys:
                universe.update(set(stgy["universe"]))
        # print(str(universe))
        portal_iqt.Env.universe = universe'''

    def load_all(self, config):
        stgys = config["stgys"]
        if config["run_type"] == RUN_TYPE.COLLECT:
            for stgy in stgys:
                if stgy["stgy_type"] == "collect":
                    stgys = [stgy]
                    break
            else:
                stgys = []
            drop_tick = False
        else:
            tmp_stgys = []
            for stgy in stgys:
                if stgy["stgy_type"] != "collect":
                    tmp_stgys.append(stgy)
            stgys = tmp_stgys
            drop_tick = True

        if len(stgys) == 0:
            return
        stgy_num = len(stgys)
        stgy_group_max_size = config["available_cpu_num"] if "available_cpu_num" in config else cpu_count()
        stgy_group_size = stgy_num if stgy_group_max_size > stgy_num else stgy_group_max_size

        logger.info("此次将加载{}个策略，分配到{}个线程执行".format(stgy_num, stgy_group_size))

        stgy_id_set = set()

        for i in range(stgy_num):
            if stgys[i]["stgy_id"] in stgy_id_set:
                raise KeyError("stgy_id[{}] is reduplicative".format(stgys[i]["stgy_id"]))
            else:
                stgy_id_set.add(stgys[i]["stgy_id"])
            mod_i = i % stgy_group_size
            if len(self._stgy_executor_list) < mod_i + 1:
                self._stgy_executor_list.append(portal_stgy.StgyExecutor(drop_tick))
            self._load(stgys[i], self._stgy_executor_list[mod_i])

        for executor in self._stgy_executor_list:
            executor.init()
            executor.start()
        portal_stgy.StgyExecutor.start_event_source()
