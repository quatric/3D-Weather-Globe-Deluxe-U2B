#import "BaseWeatherPalettePrintDataSource.h"

@implementation BaseWeatherPalettePrintDataSource

- (void)dealloc
{
	if (_entries) {
		_entries->release();
		_entries = 0;
	}
	[_printView release];
	[super dealloc];
}

- (id)initWithEntriesList:(TLArray<PointEntry *> *)aList
{
	self = [super init];
	if (self) {
		if (aList)
			aList->retain();
		_entries = aList;
	}
	return self;
}

- (BOOL)isPrintOnSinglePage
{
	return NO;
}

@end
