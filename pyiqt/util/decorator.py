#!/usr/bin/python
# -*- coding: utf-8 -*-

from threading import Thread


# 异步装饰器，即开一个线程来运行代码
def async(f):
    def wrapper(*args, **kwargs):
        thr = Thread(target=f, args=args, kwargs=kwargs)
        thr.start()

    return wrapper


# 方法4:也是方法1的升级（高级）版本,
# 使用装饰器(decorator),
# 这是一种更pythonic,更elegant的方法,
# 单例类本身根本不知道自己是单例的,因为他本身(自己的代码)并不是单例的
def singleton(cls, *args, **kw):
    instances = {}

    def _singleton():
        if cls not in instances:
            instances[cls] = cls(*args, **kw)
        return instances[cls]

    return _singleton
