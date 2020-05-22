import redis
from pyiqt.util.decorator import singleton


@singleton
class RedisFactory:
    def __init__(self):
        self._def_redis = None
        self._redis_map = {}

    def create_redis(self, host, port=6379, password=None, db=0):
        pool = redis.ConnectionPool(host=host, port=port, db=db, password=password, decode_responses=True)
        self._def_redis = redis.StrictRedis(connection_pool=pool)

    def get_redis(self) -> redis.StrictRedis:
        #     assert isinstance(r, redis.StrictRedis)
        return self._def_redis

    def create_redis_by_id(self, host, port=6379, db=0, password=None, redis_id="def_redis_id"):
        pool = redis.ConnectionPool(host=host, port=port, db=db, password=password, decode_responses=True)
        self._redis_map[redis_id] = redis.StrictRedis(connection_pool=pool)

    def get_redis_by_id(self, redis_id) -> redis.StrictRedis:
        #     assert isinstance(r, redis.StrictRedis)
        return self._redis_map[redis_id]

