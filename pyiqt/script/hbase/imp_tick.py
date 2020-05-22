#!/usr/bin/python
# -*- coding: utf-8 -*-

import sys
import os
import re
import time

import happybase

pool = happybase.ConnectionPool(size=3, host='hadoop201', table_prefix='f_tick')

INFO = 'info'
ORDER_BOOK_ID = b'info:order_book_id'
TRADING_DATE = b'info:trading_date'
DATETIME = b'info:datetime'
LAST = b'info:last'
OPEN_INSTREST = b'info:open_intrest'
OPEN_INSTREST_INCR = b'info:open_intrest_incr'
TRUNOVER = b'info:trunover'
TOTAL_TRUNOVER = b'info:total_trunover'
VOLUME = b'info:volume'
TOTAL_VOLUME = b'info:total_volume'
OPEN_VOLUME = b'info:open_volume'
CLOSE_VOLUME = b'info:close_volume'
TRADE_TYPE = b'info:trade_type'
SIDE = b'info:side'
B1 = b'info:b1'
A1 = b'info:a1'
B1_V = b'info:b1_v'
A1_V = b'info:a1_v'
OPEN = b'info:open'
HIGH = b'info:high'
LOW = b'info:low'


class Tick:
    def __init__(self, order_book_id, trading_date):
        self.order_book_id = order_book_id
        self.trading_date = trading_date
        self.open = None
        self.high = 0
        self.low = 9999999999999999
        self.total_turnover = 0
        self.total_volume = 0


def get_order_book_id(instrument_id):
    if len(instrument_id) < 4:
        return None
    if instrument_id[-4] not in '0123456789':
        instrument_id = instrument_id[:2] + '1' + instrument_id[-3:]
    else:
        instrument_id = instrument_id
    return instrument_id.upper()


p = re.compile('[-:. ]')


def process_line(batch, last_tick, line):
    words = line.strip().split(',')
    row_key = (last_tick.trading_date + p.sub('', words[2])).encode()
    last_tick.datetime = words[2]
    last_tick.last = float(words[3])
    last_tick.open_instrest = int(words[4])
    last_tick.open_instrest_incr = int(words[5])
    last_tick.turnover = float(words[6])
    last_tick.total_turnover += float(words[6])
    last_tick.volume = int(words[7])
    last_tick.total_volume += int(words[7])
    last_tick.open_volume = int(words[8]) if words[8] != '-' else -1
    last_tick.close_volume = int(words[9]) if words[9] != '-' else -1
    last_tick.trade_type = words[10]
    last_tick.side = words[11]
    last_tick.b1 = float(words[12])
    last_tick.a1 = float(words[13])
    last_tick.b1_v = int(words[14])
    last_tick.a1_v = int(words[15])
    last_tick.open = last_tick.last if last_tick.open is None else last_tick.open
    last_tick.high = last_tick.last if last_tick.high < last_tick.last else last_tick.high
    last_tick.low = last_tick.last if last_tick.low > last_tick.last else last_tick.low

    batch.put(row_key, {
        ORDER_BOOK_ID: last_tick.order_book_id,
        TRADING_DATE: last_tick.trading_date,
        DATETIME: last_tick.datetime,
        LAST: str(last_tick.last),
        OPEN_INSTREST: str(last_tick.open_instrest),
        OPEN_INSTREST_INCR: str(last_tick.open_instrest_incr),
        TRUNOVER: str(last_tick.turnover),
        TOTAL_TRUNOVER: str(last_tick.total_turnover),
        VOLUME: str(last_tick.volume),
        TOTAL_VOLUME: str(last_tick.total_volume),
        OPEN_VOLUME: str(last_tick.open_volume),
        CLOSE_VOLUME: str(last_tick.close_volume),
        TRADE_TYPE: str(last_tick.trade_type).encode('gbk'),
        SIDE: last_tick.side,
        B1: str(last_tick.b1),
        A1: str(last_tick.a1),
        B1_V: str(last_tick.b1_v),
        A1_V: str(last_tick.a1_v),
        OPEN: str(last_tick.open),
        HIGH: str(last_tick.high),
        LOW: str(last_tick.low),
    })


def process_file(order_book_id, trading_date, full_file_name):
    with pool.connection() as connection:
        try:
            enabled = connection.is_table_enabled(order_book_id)
            if not enabled:
                connection.enable_table(order_book_id)
                enabled = True
        except:
            enabled = False
        if not enabled:
            connection.create_table(
                order_book_id,
                {
                    INFO: dict(max_versions=1)
                }
            )

    start = time.clock()
    with open(full_file_name, 'r', encoding='gbk') as f:
        print('processing file: %s........' % full_file_name)
        i = 0
        last_tick = Tick(order_book_id, trading_date)
        with pool.connection() as connection:
            table = connection.table(order_book_id)
            with table.batch(batch_size=3000) as b:
                for line in f.readlines():
                    i += 1
                    if i == 1:
                        continue
                    try:
                        process_line(b, last_tick, line)
                    except Exception as ex:
                        print('line %d, content: %s, exception: %s' % (i, line, str(ex)))
                        raise ex
    print('time elapse is %f seconds' % (time.clock() - start))


def callback(base_dir, file_name):
    instrument_id, trading_date = file_name.split('_')
    trading_date = trading_date.replace('.csv', '')
    if instrument_id.find('次主力连续') >= 0:
        return
    elif instrument_id.find('主力连续') >= 0:
        order_book_id = instrument_id.replace('主力连续').upper() + '88'
    else:
        order_book_id = get_order_book_id(instrument_id)

    process_file(order_book_id, trading_date, os.path.join(base_dir, file_name))


def traverse_dir(base_dir, callback):
    for file_name in os.listdir(base_dir):
        # 忽略.开头文件
        if file_name.startswith('.'):
            continue
        full_file_name = os.path.join(base_dir, file_name)
        if os.path.isdir(full_file_name):
            traverse_dir(full_file_name, callback)
        else:
            callback(base_dir, file_name)


if __name__ == '__main__':
    if len(sys.argv) >= 2:
        base_dir = sys.argv[1]
    else:
        base_dir = '.'

    traverse_dir(base_dir, callback)
