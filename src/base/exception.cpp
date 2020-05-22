#include "exception.h"

#include <execinfo.h>
#include <cxxabi.h>

//#include <cstdlib>

#include <iostream>
#include <sstream>

 
namespace base
{
BaseException::BaseException(const std::string& msg) noexcept
    : msg_(msg),
      kFile_("<unknown file>"),
      kFunc_("<unknown func>"),
      line_(-1),
      stackTraceSize_(0)
{}
 
BaseException::~BaseException() = default;
 
void BaseException::init(const char* file, const char* func, int line)
{
    kFile_ = file;
    kFunc_ = func;
    line_ = line;
    stackTraceSize_ = backtrace(stackTrace_, MAX_STACK_TRACE_SIZE);
}
 
std::string BaseException::getClassName() const
{
    return "BaseException";
}
 
const char* BaseException::what() const noexcept
{
    return toString().c_str();
}
 
const std::string& BaseException::toString() const
{
    if (what_.empty())
    {
		std::ostringstream sstr;
        if (line_ > 0)
        {
            sstr << kFile_ << "(" << line_ << ")";
        }
        sstr <<  ": " << getClassName();
        if (!getMessage().empty())
        {
            sstr << ": " << getMessage();
        }
        sstr << "\nStack Trace:\n";
        sstr << getStackTrace();
        what_ = sstr.str();
    }
    return what_;
}
 
std::string BaseException::getMessage() const
{
    return msg_;
}
 
std::string BaseException::getStackTrace() const
{
    if (stackTraceSize_ == 0)
        return "<No stack trace>\n";
    char** strings = backtrace_symbols(stackTrace_, stackTraceSize_);
    if (strings == nullptr) // Since this is for debug only thus
                         // non-critical, don't throw an exception.
        return "<Unknown error: backtrace_symbols returned NULL>\n";
 
    std::string result;
    for (int i = 0; i < stackTraceSize_; ++i)
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
                                      nullptr, 0, &status);
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

} /* base */ 
