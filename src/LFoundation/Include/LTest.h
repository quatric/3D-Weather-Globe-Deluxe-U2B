// LTest: a self-registering test; runAllTests runs them in order and stops at
// the first that returns a non-zero status.
#pragma once

#include "LContainers.h"

namespace LF {

class LTest {
public:
	LTest();
	virtual ~LTest();
	virtual int runTest() = 0;
	virtual const char *testName() = 0;

	static int runAllTests();

	static TArray<LTest *> *sAllTests;
};

} // namespace LF
