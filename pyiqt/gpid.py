#!/usr/bin/python
# -*- coding: utf-8 -*-
from __future__ import print_function
import subprocess
import os
import sys

base_path = os.path.dirname(os.path.realpath(sys.argv[0]))
gpid_file = os.path.join(base_path, "gpid.txt")
if len(sys.argv) < 3:
    print("parameter less than 3!!!")
    exit(1)

is_shell = False
for arg in sys.argv:
    if is_shell:
        script_file = os.path.abspath(arg)
        break
    if arg == "--script":
        is_shell = True


def check_gpid(gpid):
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
        if gpid and os.getpgrp() != int(gpid) and check_gpid(gpid):
            return gpid

    return None


if _status():
    print("another service is already running...", file=sys.stderr)
    exit(1)


def _save_gpid():
    gpid = os.getpgrp()
    print("gpid={}".format(gpid))

    import re
    pattern = re.compile(r"{}:(\d+)".format(script_file))
    new_lines = ["{}:{}\n".format(script_file, gpid)]
    if os.path.exists(gpid_file):
        with open(gpid_file, 'r') as f:
            for line in f.readlines():
                if re.match(pattern, line):
                    continue
                new_lines.append(line)
    with open(gpid_file, 'w') as f:
        f.writelines(new_lines)


_save_gpid()
