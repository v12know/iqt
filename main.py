#!/usr/bin/python
# -*- coding: utf-8 -*-
import os
import sys
import setproctitle
import threading

from pyiqt.const import RUN_TYPE
from pyiqt.util.logger import LoggerFactory
from pyiqt.util.redis import RedisFactory
from pyiqt import portal_iqt as iqt
from pyiqt.ctp import portal_ctp as ctp

logger = LoggerFactory().create_logger()

app_base_dir = os.path.dirname(os.path.realpath(sys.argv[0]))
log_keep_days = 3
# 截断python日志文件
LoggerFactory().trunc_log_dir(keep_days=log_keep_days)

# 设置进程名称
setproctitle.setproctitle("main.py")

keep_alive_list = []


def term(sig_num=None, addtion=None):
    import signal
    logger.info('current pid is %s, group id is %s' % (os.getpid(), os.getpgrp()))
    os.killpg(os.getpgid(os.getpid()), signal.SIGKILL)
    signal.signal(signal.SIGTERM, None)
    signal.signal(signal.SIGINT, None)


'''
# signal.signal(signal.SIGTERM, term)
signal.signal(signal.SIGINT, term)
'''


def init_logger(config):
    logger = config["logger"]
    level = logger["level"] if "level" in logger else "info"
    base_dir = logger["base_dir"] if "base_dir" in logger else "./log"
    def_name = logger["def_name"] if "def_name" in logger else "default"
    async_size = logger["async_size"] if "async_size" in logger else 4096
    console_flag = logger["console_flag"] if "console_flag" in logger else True

    iqt.LogFactory.init(level, base_dir, def_name, async_size, console_flag)
    # 截断c++日志文件
    global log_keep_days
    LoggerFactory().trunc_log_dir(log_dir=base_dir, keep_days=log_keep_days)


def check_redis_in_use():
    from pyiqt.gpid import check_gpid
    from redis import WatchError
    key = "gpid:db:"
    r = RedisFactory().get_redis()
    with r.pipeline() as pipe:
        try:
            pipe.watch(key)
            if r.exists(key):
                gpid = r.get(key)
                b = check_gpid(gpid)
                print("gpid={}, check_gpid={}".format(gpid, b))
                if not b:
                    pipe.multi()
                    pipe.set(key, os.getpgrp())
                    pipe.expire(key, 60 * 60 * 24)
                    pipe.execute()
                return b
            else:
                pipe.multi()
                pipe.set(key, os.getpgrp())
                pipe.expire(key, 60 * 60 * 24)
                pipe.execute()
                return False
        except WatchError:
            return True


def init_py_redis(config):
    # python
    redis = config["redis"]
    # pool_id = redis["pool_id"]
    host = redis["host"]
    port = redis["port"] if "port" in redis else "6379"
    password = redis["password"] if "password" in redis else ""
    db = redis["db"]
    RedisFactory().create_redis(host, port, password, db)
    if check_redis_in_use():
        raise RuntimeError("redis host={}, port={}, db={} is already in use".format(host, port, db))


def init_redis(config):
    # cpp
    redis = config["redis"]
    # pool_id = redis["pool_id"]
    host = redis["host"]
    port = redis["port"]
    password = redis["password"] if "password" in redis else ""
    db = redis["db"]
    iqt.RedisFactory.create_redis(host, port, db, password)


def init_ctp_md_gateway(config):
    event = config["ctp"]["event"]

    broker_id = event["broker_id"]
    user_id = event["user_id"]
    password = event["password"]
    address = event["address"]

    md_gateway = ctp.CtpMdGateway(5, 1)
    global keep_alive_list
    keep_alive_list.append(md_gateway)

    md_gateway.connect(broker_id, user_id, password, address)
    return md_gateway


def init_ctp_trade_gateway(config):
    trade = config["ctp"]["trade"]
    broker_id = trade["broker_id"]
    user_id = trade["user_id"]
    password = trade["password"]
    address = trade["address"]
    pub_resume_type = trade["pub_resume_type"]
    priv_resume_type = trade["priv_resume_type"]
    # collect_pattern = trade["collect_pattern"].strip() if "collect_pattern" in trade else ""
    # run_type = config["run_type"]

    trade_gateway = ctp.CtpTradeGateway(5, 1)
    global keep_alive_list
    keep_alive_list.append(trade_gateway)
    # if run_type == RUN_TYPE.COLLECT:
    #     trade_gateway.collect_pattern = collect_pattern

    trade_gateway.connect(broker_id, user_id, password, address, pub_resume_type, priv_resume_type)
    return trade_gateway


def init_stgy(config):

    from pyiqt.stgy.stgy_loader import StgyLoader
    StgyLoader().load_all(config)


def convert_run_type(config):
    run_type = config["run_type"].upper()
    config["run_type"] = RUN_TYPE(run_type)


def launch_iqt(config):
    init_logger(config)
    init_redis(config)

    convert_run_type(config)
    iqt_config = iqt.Config()
    iqt_config.run_type = config["run_type"].name
    iqt_config.start_date = config["start_date"]
    iqt_config.future_starting_cash = config["future_starting_cash"]
    iqt_config.margin_multiplier = config["margin_multiplier"] if "margin_multiplier" in config else 1

    iqt.Env.config = iqt_config

    md_gateway = init_ctp_md_gateway(config)  # 有先后顺序
    trade_gateway = init_ctp_trade_gateway(config)

    # from pyiqt.stgy.stgy_loader import StgyLoader
    # StgyLoader().init_universe(config)  # 先将策略中的universe收集并设置

    init_stgy(config)

    cond = threading.Condition()
    cond.acquire()
    cond.wait()


def load_config(config_file):
    import json
    import re
    # p = re.compile("([^:]|^)//.*$")#去除json的//注释
    p1 = re.compile("([^:]|^)//[^\r\n]*|/\*.*?\*/", re.M | re.S)#去除json的// and /**/注释
    with open(config_file, "r") as f:
        # content = "".join(map(lambda line: p.sub(r"\1", line), f.readlines()))
        content = "".join(f.readlines())
        content = p1.sub(r"\1", content)
        # print(content)
        # exit(1)
        config_dict = json.loads(content)
    return config_dict


if __name__ == '__main__':

    if len(sys.argv) >= 1:
        config_file = sys.argv[1]
        try:
            #config_file = "./config/option_grid_rb_dev.json"
            #config_file = "./config/time_line_dev.json"
            # config_file = "./config/collect.json"
            config = load_config(config_file)
            init_py_redis(config)

            launch_iqt(config)
        except KeyboardInterrupt as e:
            pass
        except:
            logger.exception("runtime error!", exc_info=True)
        finally:
            term()
    else:
        print("""
Usage: {} CONFIG_FILE""".format(os.path.basename(__file__)))
