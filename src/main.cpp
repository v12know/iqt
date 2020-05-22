//#include <signal.h>
//#include <cstdio>
//#include <cstdlib>
//#include <climits>
//#include <ctime>
//#include <unistd.h>
//#include <execinfo.h>
#include <string>
#include <iostream>
//#include <thread>

//#include "uv.h"

//#include "ctp/md_api.h"
//#include "ctp/td_api.h"
//#include "base/exception.h"
#include "launch.h"
//#include "ctp/td_api.h"
//#include "ctp/ThostFtdcUserApiDataType.h"


using namespace std;

using namespace iqt;

void (*pResultFunc) (int elem);

extern "C" void setResultCbk(void (*pResultFunc1) (int elem)) {
	pResultFunc = pResultFunc1;
}

/*int testException() {
	MY_THROW(BaseException, "test throw");
}

int test1() {
	try {
		testException();
	} catch (BaseException &e) {
		log_error(e.what());
		exit(1);
	}
	return 0;
}*/


//uv_async_t async;
/*#include "ctp/ctp_model.h"
#include <unordered_map>
#include <memory>
#include <string>*/

int main(int argc, char *argv[]) {


	/*std::unordered_map<std::string, std::shared_ptr<iqt::ctp::MarketData>> marketDataMap_;
	std::shared_ptr<iqt::ctp::MarketData> marketData = marketDataMap_["test"];
	if (marketData == nullptr) {
		cerr << "nullptr hahaha=====" << endl;
	}
	exit(1);*/

	// pResultFunc(123);
	std::string confPath = "./config/single_grid_m_dev.xml";
	if (argc >= 2) {
		confPath = argv[1];
	}
	cerr << "confPath=" << confPath << endl;
	//signal(SIGSEGV, handler);   // install our handler
	/*try {
		int i = 1/1;
	} catch (...) {
		cout << "hehe" << endl;
		handler(0);	
	}*/

	//test1();

	Launch::instance()->init(confPath);

	//sleep(100);
	cerr << "end main" << endl;	
	return 0;
}

