#import "WeatherPalettePrintDataSourceProvider.h"
#import "WeatherPaletteShortPrintDataSource.h"
#import "WeatherPaletteExtPrintDataSource.h"

// Prints the weather of a list of locations, short or extended.
@implementation WeatherPalettePrintDataSourceProvider

- (void)dealloc
{
	releaseAndClear(_entries);
	[super dealloc];
}

+ (id)weatherPalettePrintDataSourceProviderWithEntriesList:(TLArray<PointEntry*> *)aList isPrintShortForecast:(BOOL)aForecast
{
	return [[[WeatherPalettePrintDataSourceProvider alloc] initWithEntriesList:aList isPrintShortForecast:aForecast] autorelease];
}

- (id)initWithEntriesList:(TLArray<PointEntry*> *)aList isPrintShortForecast:(BOOL)aForecast
{
	self = [super init];
	if (self) {
		LRetain(aList);
		_entries = aList;
		_isShortForecast = aForecast;
	}
	return self;
}

- (id)printDataSources
{
	Class sourceClass = _isShortForecast ? [WeatherPaletteShortPrintDataSource class] : [WeatherPaletteExtPrintDataSource class];
	return [NSArray arrayWithObject:[[[sourceClass alloc] initWithEntriesList:_entries] autorelease]];
}

- (id)hostWindowForPrintDialog
{
	return nil;
}

- (id)customPrintDialog
{
	return nil;
}

- (id)printJobTitle
{
	return @"";
}

- (void)preparePrintingDidEndSelector:(SEL)aSelector target:(id)aTarget
{
	[aTarget performSelector:aSelector];
}

@end
