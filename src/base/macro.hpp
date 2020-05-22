#ifndef _BASE_MACRO_HPP_
#define _BASE_MACRO_HPP_

// Put this in the private: declarations for a class to be unassignable.
#define DISALLOW_ASSIGN(TypeName) void operator=(const TypeName&) = delete;


// A macro to disallow the evil copy constructor and operator= functions
// This should be used in the private: declarations for a class
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
	TypeName(const TypeName&) = delete;    \
	void operator=(const TypeName&) = delete


// A macro to disallow all the implicit constructors, namely the
// default constructor, copy constructor and operator= functions.
//
// This should be used in the private: declarations for a class
// that wants to prevent anyone from instantiating it. This is
// especially useful for classes containing only static methods.
#define DISALLOW_IMPLICIT_CONSTRUCTORS(TypeName) \
	TypeName() = delete;                         \
	DISALLOW_COPY_AND_ASSIGN(TypeName)
	
/* This macro looks complicated but it's not: it calculates the address
 * of the embedding struct through the address of the embedded struct.
 * In other words, if struct A embeds struct B, then we can obtain
 * the address of A by taking the address of B and subtracting the
 * field offset of B in A.
 */
#define CONTAINER_OF(ptr, type, field)                                        \
	((type *) ((char *) (ptr) - ((char *) &((type *) 0)->field)))

#endif /* end of include guard: _BASE_MACRO_HPP_ */
