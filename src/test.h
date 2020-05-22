//
// Created by Carl Yang on 2020/3/6.
//

#ifndef IQT_TEST_H
#define IQT_TEST_H
/**
 * calibrate date independent of market open/close time.
 * pros:
 * no need to calculate every tick's date
 * cons:
 * dependent of market open/close time.
 * @param update_time tick's update time
 * @return calibrated date
 */
base::date calib_date(const base::daytime& update_time) {

}
/**
 * calibrate date dependent of market open/close time.
 * pros:
 * cons:
 * dependent of market open/close time.
 * 1. when the date switch period (23:59:30~00:00:30), every tick's date will be calculated;
 * 2. if there's no market data in the date switch period, _now_time variable won't be updated, so it depends system's restart every morning.
 * @param update_time tick's update time
 * @return calibrated date
 */
base::date calib_date_by_mkt_time(const base::daytime& update_time) {

}
#endif //IQT_TEST_H
