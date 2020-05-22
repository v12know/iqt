import json
from kafka import KafkaProducer
import time

producer = KafkaProducer(bootstrap_servers='hadoop201:9092,hadoop202:9092,hadoop203:9092',
                         value_serializer=lambda v: json.dumps(v).encode('utf-8'))

ticks = []
with open('./tick.txt', 'r') as f:
    for line in f:
        ticks.append(json.loads(line))

for tick in ticks:
    producer.send("test", key=tick["order_book_id"].encode('utf-8'), value=tick)
    time.sleep(0.1)