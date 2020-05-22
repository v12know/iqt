#ifndef _SMART_ASSERT_H_
#define _SMART_ASSERT_H_

/*
#include "base/macro.h"

#define SMART_ASSERT_A(x) SMART_ASSERT_OP(x, B)
#define SMART_ASSERT_B(x) SMART_ASSERT_OP(x, A)
#define SMART_ASSERT_OP(x, next)				\
	SMART_ASSERT_A.printCurrentVal((x), #x).SMART_ASSERT_##next

#define SMART_ASSERT(expr)						\
	if( (expr) ) ;								\
	else AssertContext.instance()->printExpr(#expr).printContext(__FILE__, __LINE__).SMART_ASSERT_A
class Assert
{
public:
	Assert *instance()
	{
		static Assert sAssert;
		return &sAssert;
	}

	DISALLOW_COPY_AND_ASSIGN(Assert);
	
	Assert &setExpr(const char *expr)
	{
		context_setExpr(expr);
		return *this;
	}

	Assert &print_current_val(bool, const char*);

	Assert &SMART_ASSERT_A;
	Assert &SMART_ASSERT_B;
	//whatever member functions

private:
	AssertContext context_;
};

*/
#endif /* end of include guard: _SMART_ASSERT_H_ */
