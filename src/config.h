#ifndef _IQT_CONFIG_H_
#define _IQT_CONFIG_H_

#include <string>
#include <vector>
#include "base/macro.hpp"
#include "stgy/stgy_model.h"

namespace cereal
{
  template <class T>
  class construct;


} /* cereal */ 

namespace iqt
{

struct Logger
{
	std::string level;
	std::string logFile;
	int asyncSize;
	bool consoleFlag;
	
	template<class Archive>
    void serialize(Archive & ar);

	bool operator==(Logger const & l)
	{
		return level == l.level &&
			logFile == l.logFile &&
			asyncSize == l.asyncSize &&
			consoleFlag == l.consoleFlag;
	}

};

struct MdInfo {
	std::string brokerId;
	std::string userId;
	std::string password;
	std::string frontAddr;
	//std::string instrumentIds;//合约ID列表，用,分隔
	template<class Archive>
    void serialize(Archive & ar);
};

struct TdInfo {
	std::string acnId;//绑定帐号
	std::string brokerId;//经纪公司代码
	std::string userId;//用户代码
	std::string password;//密码
	std::string frontAddr;//前置机IP地址
	std::string pubResumeType;//公共流重传类型
	std::string privResumeType;//私有流重传类型
	int tdAmt;//启动的交易实例数量
	std::string validTimeSpans;//交易的有效时间段
	double capitalUpperRate;//绑定帐号的资金上限比率
	template<class Archive>
    void serialize(Archive & ar);
};

struct Redis {
	std::string redisId;
	std::string host;
	int port;
	std::string password;
	int dbIndex;
	template<class Archive>
    void serialize(Archive & ar);
};


struct Config
{
	static Config *instance() {
		static Config sConfig;
		return &sConfig;
	}
	~Config() {
	}

	template<class Archive>
	void serialize(Archive & ar);

	template <class Archive>
	static void load_and_construct( Archive & ar, cereal::construct<Config> & construct )
	{
		Config::instance()->serialize(ar);
	}

	bool operator==(Config const & c)
	{
		return logger == c.logger;
	}

	DISALLOW_COPY_AND_ASSIGN(Config);
	static void load(const std::string &configFile = "../config/config.xml");
	static void save(const std::string &configFile = "../config/config.xml");
/** data **/
	Logger logger;
	MdInfo mdInfo;
	TdInfo tdInfo;
	std::vector<Redis> redises;
	std::vector<stgy::Grid> grids;
	std::vector<stgy::OptionGrid> optionGrids;
private:
	Config() {}
};


} /* iqt */ 
#endif /* end of include guard: _IQT_CONFIG_H_ */
