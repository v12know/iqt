--创建tick表语句
create table future_tick.ma709 (
    order_book_id string,
    datetime timestamp,
    date int,
    time int,
    open double,
    last double,
    low double,
    high double,
    prev_close double,
    total_volume int,
    total_turnover double,
    open_interest double,
    prev_settlement double,

    b1 double,
    b2 double,
    b3 double,
    b4 double,
    b5 double,
    b1_v int,
    b2_v int,
    b3_v int,
    b4_v int,
    b5_v int,

    a1 double,
    a2 double,
    a3 double,
    a4 double,
    a5 double,
    a1_v int,
    a2_v int,
    a3_v int,
    a4_v int,
    a5_v int,

    limit_up double,
    limit_down double
) partitioned by (trading_date int)
--clustered by (datetime) sorted by (datetime) into 8 buckets
stored as orc;


create table future_1m.ma709 (
    order_book_id string,
    datetime timestamp,
    date int,
    time int,
    open double,
    close double,
    low double,
    high double,
    total_volume int,
    total_turnover double,
    open_interest double,
    basis_spread double,
    settlement double,
    prev_settlement double,

    limit_up double,
    limit_down double
) partitioned by (trading_date int)
--clustered by (date) sorted by (datetime) into 4 buckets
stored as orc;
