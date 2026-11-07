// LTest
//
// LF::LTest.

#include "LTest.h"

namespace LF {

TArray<LTest *> *LTest::sAllTests = 0;

LTest::LTest()
{
	if (sAllTests == 0)
		sAllTests = new TArray<LTest *>;
	LTest *self = this;
	sAllTests->addObject(self);
}

LTest::~LTest()
{
	LTest *self = this;
	sAllTests->removeObject(self);
}

int LTest::runAllTests()
{
	int n = sAllTests ? sAllTests->count() : 0;
	for (int i = 0; i < n; i++) {
		LTest *test = (*sAllTests)[i];
		LLog("Running test %s...\n", test->testName());
		int status = test->runTest();
		LLog("Test passed with status %d\n", status);
		if (status != 0) {
			LLog("TERMINATING!!!\n\n");
			return status;
		}
		LLog("\n");
	}
	return 0;
}

} // namespace LF
