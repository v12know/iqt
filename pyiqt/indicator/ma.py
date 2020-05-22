# 将均线计算改写为高速算法
def ma_online(data, ma_length):
    ma = []
    data_window = data[:ma_length]
    test_data = data[ma_length:]

    # 缓存的窗口内数据求和结果
    sum_buffer = 0

    for new_tick in test_data:
        old_tick = data_window.pop(0)
        data_window.append(new_tick)

        # 如果缓存结果为空，则先通过遍历求第一次结果
        if not sum_buffer:
            sum_tick = 0
            for tick in data_window:
                sum_tick += tick
            ma.append(sum_tick / ma_length)

            # 将求和结果缓存下来
            sum_buffer = sum_tick
        else:
            # 这里的算法将计算复杂度从O(n)降低到了O(1)
            sum_buffer = sum_buffer - old_tick + new_tick
            ma.append(sum_buffer / ma_length)

    return ma
