# -*- coding: utf-8 -*-

from pymongo import MongoClient

mongodb_host = '192.168.0.202'
mongodb_port = 27017
mongodb_user = 'vnpy'
mongodb_pwd = 'vnpy'


class mongodb_client(object):
    def __init__(self):
        self.dbClient = None

    # ----------------------------------------------------------------------
    def writeLog(self, content):
        """日志"""
        print(content)

    # ----------------------------------------------------------------------
    def dbConnect(self):
        """连接MongoDB数据库"""
        if not self.dbClient:

            try:
                # 设置MongoDB操作的超时时间为0.5秒
                self.dbClient = MongoClient(mongodb_host, mongodb_port, serverSelectionTimeoutMS=500)

                # 这里使用了ticks这个库来验证用户账号和密码
                self.dbClient.ticks.authenticate(mongodb_user, mongodb_pwd, mechanism='SCRAM-SHA-1')

                # 调用server_info查询服务器状态，防止服务器异常并未连接成功
                self.dbClient.server_info()

                self.writeLog(u'MongoDB连接成功')
            except Exception as ex:
                self.writeLog(u'MongoDB连接失败{0}'.format(ex))

    # ----------------------------------------------------------------------
    def dbInsert(self, dbName, collectionName, d):
        """向MongoDB中插入数据，d是具体数据"""
        if self.dbClient:
            db = self.dbClient[dbName]
            collection = db[collectionName]
            collection.insert(d)

    # ----------------------------------------------------------------------
    def dbInsertMany(self, dbName, collectionName, dataList):
        """向MongoDB中插入Multi数据，dataList是具体数据List"""
        if self.dbClient:
            db = self.dbClient[dbName]
            collection = db.getCollection(collectionName)
            collection.insertMany(dataList)

    # ----------------------------------------------------------------------
    def dbQuery(self, dbName, collectionName, d):
        """从MongoDB中读取数据，d是查询要求，返回的是数据库查询的指针"""
        if self.dbClient:
            db = self.dbClient[dbName]
            collection = db[collectionName]
            cursor = collection.find(d)
            return cursor
        else:
            return None


mc = mongodb_client()
mc.dbConnect()

cursor = mc.dbQuery("ticks", "RB01", {"date": "20170306", "time": "20:59:00.000000"})
for i in cursor:
    print(i)