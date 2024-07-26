//NOTE using tcc to test C compatibility:
//tcc examples/tests/tests.c -Isrc -g -oexamples/tests/build/debug/test

#define DESHI_TESTS

//#define DESHI_MATH_DISABLE_LIBC
//#define DESHI_MATH_DISABLE_SSE
#define DESHI_MATH_DISABLE_CPP
#include "core/math.h"

int main(){
	TEST_deshi_math();
}
