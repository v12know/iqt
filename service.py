#!/usr/bin/python
# -*- coding: utf-8 -*-
from __future__ import print_function
import sys
reload(sys)
sys.setdefaultencoding('utf-8')
import time

# import commands
import os
import subprocess

python = "/usr/bin/python"
bash = "/bin/bash"
valid_time_span = "20:45:00~16:00:00"
base_path = os.path.dirname(os.path.realpath(__file__))
gpid_file = os.path.join(base_path, "gpid.txt")
tmp_cron_file = os.path.join(base_path, "cron.tmp")


script_file = None
nohup_command = None
cron_content = None
service_file = os.path.realpath(__file__)


def _generate_command(script_name):
    global script_file
    global nohup_command
    global cron_content
    error_file = os.path.join(base_path, "cerr.log")
    null_file = "/dev/null"
    if os.path.isabs(script_name):
        script_file = os.path.abspath(script_name)
    else:
        script_file = os.path.abspath(os.path.join(base_path, script_name))
    if script_file.endswith(".sh"):
        script_type = bash
    elif script_file.endswith(".py"):
        script_type = python
    else:
        raise RuntimeError("unknown script type, script file: {}".format(script_file))
    nohup_command = "nohup {} {} >{} 2>{} &".format(script_type, script_file, null_file, error_file)
    # cron_content = "* * * * * . ~/.bash_profile && {} {} schedule >{} 2>{} &".format(python, service_file, shell_file, null_file, null_file)
    cron_content = "* * * * * {} {} {} schedule >{} 2>{}".format(python, service_file, script_file, null_file, null_file)


def _check_gpid(gpid):
    try:
        p = subprocess.Popen(["ps", "-A", "-o", "pgrp="], stdin=subprocess.PIPE,
                stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=False)
        returncode = p.wait()
    except OSError as e:
        print("cant't find shell command ps", file=sys.stderr)
        exit(1)
    try:
        p2 = subprocess.Popen("uniq", stdin=p.stdout, stdout=subprocess.PIPE,
                stderr=subprocess.PIPE, shell=False)
        returncode = p2.wait()
    except OSError as e:
        print("cant't find shell command uniq", file=sys.stderr)
        exit(1)
    for i in p2.stdout.readlines():
        if i.decode().strip() == gpid:
            # print(i.decode())
            return True
    return False


def _status():
    if os.path.exists(gpid_file):
        import re
        pattern = re.compile(r"{}:(\d+)".format(script_file))
        # print(gpid_file)
        gpid = None
        with open(gpid_file, 'r') as f:
            for line in f.readlines():
                matcher = re.match(pattern, line)
                if matcher:
                    gpid = matcher.group(1)
                    break
            # print(gpid)
        if gpid and _check_gpid(gpid):
            return gpid

    return None


def _check_stop_time():
    time_span_list = []
    for time_span in valid_time_span.split(","):
        if time_span.strip():
            time_pair = tuple([item.strip() for item in time_span.split("~")])
            time_span_list.append(time_pair)
    time_span_list.sort(key=lambda pair: pair[0])
    if len(time_span_list) == 0:
        return False
    import datetime
    now_time = datetime.datetime.now()
    now_time_str = now_time.strftime('%H:%M:%S')
    stop_flag = True
    if time_span_list[0][0] > time_span_list[-1][1] > now_time_str:
        stop_flag = False
    else:
        watch_flag = False
        for item in time_span_list:
            if item[0] > now_time_str:
                stop_flag = True
                watch_flag = True
                break
            if item[1] > now_time_str:
                stop_flag = False
                watch_flag = True
                break

        if not watch_flag:
            if time_span_list[-1][1] < time_span_list[0][0]:
                stop_flag = False
            else:
                stop_flag = True

    return stop_flag


def _start():
    gpid = _status()
    if _check_stop_time():
        if gpid:
            print("it's not in valid time span, will kill the service[gpid={}]".format(gpid), file=sys.stderr)
            import signal
            # 杀死进程组
            os.killpg(int(gpid), signal.SIGKILL)
            while _status():
                time.sleep(1)
            print("already stop the service[gpid={}]".format(gpid), file=sys.stderr)
        else:
            print("it's not in valid time span, can't start the service", file=sys.stderr)
    else:
        if not gpid:
            print("it's in valid time span, will start the service", file=sys.stderr)
            os.popen(nohup_command)

            while True:
                gpid = _status()
                if gpid:
                    break
                time.sleep(1)
            print("already start the service[gpid={}]".format(gpid), file=sys.stderr)
        else:
            print("it's in valid time span, the service is running...", file=sys.stderr)


def schedule():
    print("======schedule========", file=sys.stderr)
    _start()


def status():
    print("======status========", file=sys.stderr)

    gpid = _status()
    if gpid:
        print("the service[gpid={}] is running...".format(gpid), file=sys.stderr)
    else:
        print("the service is not running...", file=sys.stderr)


# operate的可选字符串为：add, del
def operate_crontab(operate):
    try:
        p = subprocess.Popen(["crontab", "-l"], stdin=subprocess.PIPE,
                             stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=False)
        returncode = p.wait()
    except OSError as e:
        print("cant't find shell command crontab", file=sys.stderr)
        exit(1)

    remain_cron_list = []
    exist_flag = False
    for i in p.stdout.readlines():
        if i.decode("utf-8").find("{} {} schedule".format(service_file, script_file)) >= 0:
            old_cron_content = i.decode("utf-8")
            exist_flag = True
        else:
            remain_cron_list.append(i.decode("utf-8"))

    if operate == "add" and not exist_flag:
        remain_cron_list.append(cron_content)
        remain_cron_list.append("\n")
        with open(tmp_cron_file, 'wb') as f:
            for i in remain_cron_list:
                f.write(i.encode("utf-8"))
        os.popen("crontab {}".format(tmp_cron_file))
        print("add new crontab item: {}".format(cron_content), file=sys.stderr)

    if operate == "del" and exist_flag:
        with open(tmp_cron_file, 'wb') as f:
            for i in remain_cron_list:
                f.write(i.encode("utf-8"))
        os.popen("crontab {}".format(tmp_cron_file))
        print("del old crontab item: {}".format(old_cron_content), file=sys.stderr)

    # os.remove(tmp_cron_file)


def start():
    print("======start========", file=sys.stderr)
    operate_crontab("add")
    _start()
    print("start service done!!!", file=sys.stderr)


def _stop():
    gpid = _status()
    if gpid:
        import signal
        # 杀死进程组
        os.killpg(int(gpid), signal.SIGKILL)
        while _status():
            time.sleep(1)
        print("already stop the service[gpid={}]".format(gpid), file=sys.stderr)
    else:
        print("the service is not running...", file=sys.stderr)


def stop():
    print("======stop========", file=sys.stderr)
    operate_crontab("del")
    _stop()
    print("stop service done!!!", file=sys.stderr)


def restart():
    print("======restart========", file=sys.stderr)
    _stop()
    time.sleep(1)
    operate_crontab("add")
    _start()
    print("restart service done!!!", file=sys.stderr)


if __name__ == '__main__':
    if len(sys.argv) >= 3:
        script = sys.argv[1]
        cmd = sys.argv[2]
        _generate_command(script)
    else:
        cmd = ''

    if cmd == 'status':
        status()
    elif cmd == 'start':
        start()
    elif cmd == 'stop':
        stop()
    elif cmd == 'restart':
        restart()
    elif cmd == 'schedule':
        schedule()
    else:
        print("""
Usage: {} SCRIPT COMMAND
run a activate script(shell, python, etc) periodically by added to crontab.
SCRIPT is a script which call the file including main function
COMMAND consist of status|start|stop|restart
""".format(os.path.basename(__file__)))
