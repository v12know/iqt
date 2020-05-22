#ifndef _SIG_CATCH_H_
#define _SIG_CATCH_H_

#include <signal.h>
#include <execinfo.h>
#include <string>
#include <sstream>
#include <iostream>
#include <unistd.h>


//#define __USE_GNU
//#include <sys/ucontext.h>

#include <ucontext.h>
#ifndef __USE_GNU
#error "__USE_GNU HAS been undefined"
#endif
#include <cxxabi.h>

#ifdef __x86_64__
#define REG_EIP REG_RIP
#endif

#ifndef _SYS_UCONTEXT_H
#error "sys/context HAS NOT BEEN INCLUDED"
#endif

#include "base/log.h"

static void init_sig_catch(void);
class C_SIG_CATCH{
public:
	C_SIG_CATCH(){
		init_sig_catch();
	}
};
static C_SIG_CATCH C_sig_catch;


std::string print_trace()
{
	enum { MAX_STACK_TRACE_SIZE = 50 };
	void* stackTrace_[MAX_STACK_TRACE_SIZE];
	size_t stackTraceSize_;
    stackTraceSize_ = backtrace(stackTrace_, MAX_STACK_TRACE_SIZE);
    if (stackTraceSize_ == 0)
        return "<No stack trace>\n";
    char** strings = backtrace_symbols(stackTrace_, stackTraceSize_);
    if (strings == NULL) // Since this is for debug only thus
                         // non-critical, don't throw an exception.
        return "<Unknown error: backtrace_symbols returned NULL>\n";
 
    std::string result;
    for (size_t i = 0; i < stackTraceSize_; ++i)
    {
        std::string mangledName = strings[i];
        std::string::size_type begin = mangledName.find('(');
        std::string::size_type end = mangledName.find('+', begin);
        if (begin == std::string::npos || end == std::string::npos)
        {
            result += mangledName;
            result += '\n';
            continue;
        }
        ++begin;
        int status;
        char* s = abi::__cxa_demangle(mangledName.substr(begin, end-begin).c_str(),
                                      NULL, 0, &status);
        if (status != 0)
        {
            result += mangledName;
            result += '\n';
            continue;
        }
        std::string demangledName(s);
        free(s);
        // Ignore BaseException::init so the top frame is the
        // user's frame where this exception is thrown.
        //
        // Can't just ignore frame#0 because the compiler might
        // inline BaseException::init.
        result += mangledName.substr(0, begin);
        result += demangledName;
        result += mangledName.substr(end);
        result += '\n';
    }
    free(strings);
    return result;

}


void bt_sighandler(int sig, siginfo_t *info,
                   void *secret) {
	std::ostringstream os;
	os << "\n<<< SIGNAL HANDLER >>>\n";
	os << print_trace();
	ucontext_t *uc = (ucontext_t *)secret;

	/* Do something useful with siginfo_t */
	if (sig == SIGSEGV)
		os << "Got signal " << sig << ", faulty address is " << info->si_addr << ", "
			"from " << uc->uc_mcontext.gregs[REG_EIP] << "\n";
	else
		os << "Got signal " << sig << "#92;\n";

	log_trace(os.str());
    base::LogFactory::flush_all();

    killpg(getpgrp(), SIGTERM);
}


inline static void init_sig_catch()
{
    /* Install our signal handler */
    printf("initialising signal handlers... \n");
    struct sigaction sa;

    sa.sa_sigaction = bt_sighandler;
    sigemptyset (&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_SIGINFO;


    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGABRT, &sa, NULL);
    //sigaction(SIGINT, &sa, NULL);
    sigaction(SIGBUS, &sa, NULL);
    sigaction(SIGSTKFLT, &sa, NULL);
    sigaction(SIGSYS, &sa, NULL);

    printf("signal handlers initialised successfully \n");
}


#endif /* end of include guard: _SIG_CATCH_H_ */
