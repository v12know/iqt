#ifndef _BASE_EXCEPTION_H_
#define _BASE_EXCEPTION_H_

#include <exception>
#include <string>

#define MY_THROW(ExClass, args...)                             \
    do                                                         \
    {                                                          \
        ExClass e(args);                                       \
        e.init(__FILE__, __PRETTY_FUNCTION__, __LINE__);       \
        throw e;                                               \
    }                                                          \
    while (false)

#define MY_DEFINE_EXCEPTION(ExClass, Base)                     \
    explicit ExClass(const std::string& msg = "") throw()      \
        : Base(msg)                                            \
    {}                                                         \
                                                               \
    ~ExClass() throw() = default;                              \
                                                               \
    /* override */ std::string getClassName() const override   \
    {                                                          \
        return #ExClass;                                       \
    }

namespace base {

    class BaseException : public std::exception {
    public:
        explicit BaseException(const std::string &msg = "") noexcept;

        ~BaseException() throw() override;

        void init(const char *file, const char *func, int line);

        virtual std::string getClassName() const;

        virtual std::string getMessage() const;

        const char *what() const noexcept override;

        const std::string &toString() const;

        std::string getStackTrace() const;

    protected:
        std::string msg_;
        const char *kFile_;
        const char *kFunc_;
        int line_;

    private:
        enum {
            MAX_STACK_TRACE_SIZE = 50
        };
        void *stackTrace_[MAX_STACK_TRACE_SIZE];
        int stackTraceSize_;
        mutable std::string what_;
    };

//class DerivedException : public BaseException
//{
//public:
    //MY_DEFINE_EXCEPTION(DerivedException, BaseException);
//};


    class ReqException: public BaseException {
    public:
        MY_DEFINE_EXCEPTION(ReqException, BaseException);
    };

    class RspException: public BaseException {
    public:
        MY_DEFINE_EXCEPTION(RspException, BaseException);
    };
} /* base */
#endif /* end of include guard: _BASE_EXCEPTION_H_ */
