#!/usr/bin/python
# -*- coding: utf-8 -*-


class Observer(object):
    def update(self, *args, **kwargs):
        pass


class TickObserver(Observer):
    def __init__(self, target):
        self.target = target

    def update(self, *args, **kwargs):
        self.target.appendTick(kwargs["tick"])


class BarObserver(Observer):
    def __init__(self, target):
        self.target = target

    def update(self, *args, **kwargs):
        self.target.appendBar(kwargs["bar"], kwargs["barList"])
