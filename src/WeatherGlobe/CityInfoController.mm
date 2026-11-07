#import "CityInfoController.h"
#import "CityInfoView.h"
#import "CPStatusBarView.h"
#import "CPExpandedForecastView.h"
#import "CPWeatherDescriptionView.h"
#import "CPCurrentConditionsView.h"
#import "CPMainPropertyView.h"

NSString *_kCIViewSizeDidChangeNotification = @"CIViewSizeDidChangeNotification";

// One city's info panel: lines of property views (title, conditions,
// description, forecast, status bar) stacked in a CityInfoView.
@implementation CityInfoController

- (id)initWithCityEntry:(PointEntry *)aEntry
{
	if (!aEntry) {
		[self release];
		return nil;
	}
	self = [super init];
	if (self) {
		_entry = (PointEntry *)aEntry->retain();
		[self _createAllViews];
		[self reconstructMainView];
	}
	return self;
}

- (void)dealloc
{
	LNotificationCenter::center()->removeObserver(self);
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	[_mainView removeFromSuperview];
	[_mainView release];
	[_contentView release];
	[_lineViewsContainers release];
	[_cityPropertyViews release];
	if (_entry) {
		_entry->release();
		_entry = 0;
	}
	[super dealloc];
}

- (id)mainView
{
	return _mainView;
}

- (NSSize)sizeForBestFit
{
	return _sizeForBestFit;
}

- (PointEntry *)cityEntry
{
	return _entry;
}

- (void)setMenu:(id)aMenu
{
	[_mainView setMenu:aMenu];
	[_contentView setMenu:aMenu];
	[_lineViewsContainers makeObjectsPerformSelector:@selector(setMenu:) withObject:aMenu];
	NSUInteger count = [_cityPropertyViews count];
	for (NSUInteger i = 0; i < count; i++)
		[[_cityPropertyViews objectAtIndex:i] makeObjectsPerformSelector:@selector(setMenu:) withObject:aMenu];
}

// One array of items per property view class.
+ (id)optionsMenuItems
{
	NSArray *classes = [self _propertyViewClassesForMenu];
	NSMutableArray *items = [NSMutableArray array];
	NSUInteger count = [classes count];
	for (NSUInteger i = 0; i < count; i++)
		[items addObject:[[classes objectAtIndex:i] optionsMenuItems]];
	return items;
}

- (void)manualUpdateWeather
{
	_entry->manualUpdateOfScheduledReports();
}

// The classes of each line, top to bottom.
+ (id)_allPropertyViewClasses
{
	return [NSArray arrayWithObjects:
		[NSArray arrayWithObjects:[CPMainPropertyView class], [CPCurrentConditionsView class], nil],
		[NSArray arrayWithObjects:[CPWeatherDescriptionView class], nil],
		[NSArray arrayWithObjects:[CPExpandedForecastView class], nil],
		[NSArray arrayWithObjects:[CPStatusBarView class], nil], nil];
}

+ (id)_propertyViewClassesForMenu
{
	return [NSArray arrayWithObjects:[CPMainPropertyView class], [CPCurrentConditionsView class],
		[CPExpandedForecastView class], nil];
}

// The views of each line; the first and last lines are textured (title
// and status bar backgrounds).
- (id)_allPropertyViews
{
	NSArray *classes = [CityInfoController _allPropertyViewClasses];
	NSMutableArray *views = [NSMutableArray array];
	NSMutableArray *lines = [NSMutableArray new];
	NSUInteger count = [classes count];
	for (NSUInteger i = 0; i < count; i++) {
		NSView *line;
		if (i == 0 || i == count - 1) {
			NSImage *image = [NSImage imageNamed:i == 0 ? @"city_info_title" : @"city_info_status_bar"];
			_WGCITexturedView *textured = [[_WGCITexturedView new] autorelease];
			[textured setImage:image forTexture:4];
			[textured setCompositeOperation:NSCompositingOperationCopy];
			line = textured;
		} else {
			line = [[NSView new] autorelease];
		}
		[lines addObject:line];
		NSArray *lineClasses = [classes objectAtIndex:i];
		NSMutableArray *lineViews = [NSMutableArray array];
		NSUInteger classesCount = [lineClasses count];
		for (NSUInteger j = 0; j < classesCount; j++) {
			CityPropertyGenericView *view = [[[lineClasses objectAtIndex:j] alloc] initWithCityEntry:_entry];
			[lineViews addObject:view];
			[view release];
		}
		[views addObject:lineViews];
	}
	_lineViewsContainers = lines;
	return views;
}

- (void)_createAllViews
{
	_cityPropertyViews = [[self _allPropertyViews] retain];
	_contentView = [NSView new];
	[_contentView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
	[_contentView setPostsFrameChangedNotifications:NO];
	_mainView = [CityInfoView new];
	[_mainView setTitlePosition:NSNoTitle];
	[_mainView setContentView:_contentView];
	[_mainView setOwner:self];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_allignViews)
		name:_kCIViewSizeDidChangeNotification object:_mainView];
}

// The lines stacked from the top, full width.
- (void)_allignViews
{
	NSRect content = [_contentView frame];
	CGFloat y = content.size.height;
	NSUInteger count = [_lineViewsContainers count];
	for (NSUInteger i = 0; i < count; i++) {
		NSView *line = [_lineViewsContainers objectAtIndex:i];
		CGFloat height = [line frame].size.height;
		y -= height;
		[line setFrame:NSMakeRect(0.0, y, content.size.width, height)];
	}
}

// Lays the enabled views out in their lines (centered vertically), drops
// the empty lines, merges the second line into the title one and sizes the
// main view to fit.
- (void)reconstructMainView
{
	[_mainView setPostsFrameChangedNotifications:NO];
	_sizeForBestFit = NSZeroSize;
	NSUInteger count = [_cityPropertyViews count];
	for (NSUInteger i = 0; i < count; i++) {
		NSArray *views = [_cityPropertyViews objectAtIndex:i];
		NSView *line = [_lineViewsContainers objectAtIndex:i];
		NSSize lineSize = NSZeroSize;
		NSUInteger viewsCount = [views count];
		for (NSUInteger j = 0; j < viewsCount; j++) {
			CityPropertyGenericView *view = [views objectAtIndex:j];
			if ([view enabled]) {
				NSSize size = [view sizeForBestFit];
				lineSize.width += size.width;
				lineSize.height = MAX(lineSize.height, size.height);
			}
		}
		if (!NSEqualSizes([line frame].size, lineSize)) {
			[_contentView setNeedsDisplayInRect:[line frame]];
			[line setFrameSize:lineSize];
		}
		CGFloat x = 0.0;
		for (NSUInteger j = 0; j < viewsCount; j++) {
			CityPropertyGenericView *view = [views objectAtIndex:j];
			if (![view enabled]) {
				[view removeFromSuperview];
				continue;
			}
			if ([view superview] != line)
				[line addSubview:view];
			NSSize size = [view sizeForBestFit];
			[view setFrame:NSMakeRect(x, ceilf((lineSize.height - size.height) * 0.5f), size.width, size.height)];
			x += size.width;
		}
		if (lineSize.height == 0.0) {
			[line removeFromSuperview];
		} else {
			_sizeForBestFit.height += lineSize.height;
			_sizeForBestFit.width = MAX(_sizeForBestFit.width, lineSize.width);
			if ([line superview] != _contentView)
				[_contentView addSubview:line];
		}
	}
	if ([_lineViewsContainers count] > 1) {
		NSView *first = [_lineViewsContainers objectAtIndex:0];
		NSView *second = [_lineViewsContainers objectAtIndex:1];
		NSRect firstFrame = [first frame];
		NSRect secondFrame = [second frame];
		CGFloat width = firstFrame.size.width;
		if (secondFrame.size.width < firstFrame.size.width)
			[second setFrame:NSMakeRect(secondFrame.origin.x, secondFrame.origin.y, firstFrame.size.width,
				secondFrame.size.height)];
		else
			width = secondFrame.size.width;
		[first setFrame:NSMakeRect(firstFrame.origin.x, firstFrame.origin.y, width,
			secondFrame.size.height + firstFrame.size.height)];
		while ([[second subviews] count])
			[first addSubview:[[second subviews] objectAtIndex:0]];
		[second setFrame:NSZeroRect];
	}
	_sizeForBestFit.width = (int)_sizeForBestFit.width + 1;
	_sizeForBestFit.height = (int)_sizeForBestFit.height + 1;
	[_contentView setFrameSize:_sizeForBestFit];
	[_mainView setAutoresizesSubviews:NO];
	[_mainView setFrameFromContentFrame:[_contentView frame]];
	[_mainView setAutoresizesSubviews:YES];
	_sizeForBestFit = [_mainView frame].size;
	[self _allignViews];
	[_mainView setPostsFrameChangedNotifications:YES];
}

@end
