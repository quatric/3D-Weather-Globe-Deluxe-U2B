#import "WGTitledViewContainer.h"
#import "WGTitledView.h"
#import "WGSplitView.h"
#import "WGSplitter.h"
#import "NSView+WGExtensions.h"

// Views stacked in a split view, each wrapped in a WGTitledView and kept in
// order of weight.
@implementation WGTitledViewContainer

- (id)splitView
{
	return splitView;
}

// The index of the titled view holding the view, or NSNotFound.
- (NSUInteger)indexOfSubview:(id)aSubview
{
	NSArray *subviews = [splitView subviews];
	NSUInteger count = [subviews count];
	for (NSUInteger i = 0; i < count; i++)
		if ([[subviews objectAtIndex:i] contentView] == aSubview)
			return i;
	return NSNotFound;
}

// Where a view of the weight would go.
- (NSUInteger)indexForWeight:(NSUInteger)aWeight
{
	NSArray *sorted = [(NSArray *)[splitView subviews] sortedArrayUsingFunction:_compareOrderWeight context:0];
	NSUInteger count = [[splitView subviews] count];
	NSUInteger i = 0;
	for (; i < count; i++)
		if (aWeight < [[sorted objectAtIndex:i] orderWeight])
			break;
	return i;
}

- (void)addSubview:(id)aSubview
{
	[self addSubview:aSubview withAttributes:nil];
}

// The first titled view's splitter drags the enclosing split view.
- (BOOL)_addSubview:(id)aSubview withAttributes:(id)aAttributes
{
	if (!aSubview || [self indexOfSubview:aSubview] != NSNotFound)
		return NO;
	WGTitledView *titled = [[WGTitledView alloc] initWithFrame:[aSubview bounds]];
	[titled setDelegate:self];
	NSSplitView *enclosing = [[splitView subviews] count] ? splitView : [splitView wgEnclosingSplitView];
	[[titled splitter] setEnclosingSplitView:enclosing];
	[titled setContentView:aSubview attributes:aAttributes];
	[splitView addSubview:titled];
	[titled release];
	return YES;
}

- (void)addSubview:(id)aSubview withAttributes:(id)aAttributes
{
	if ([self _addSubview:aSubview withAttributes:aAttributes])
		[splitView sortSubviewsUsingFunction:(NSComparisonResult(*)(__kindof NSView *, __kindof NSView *, void *))_compareOrderWeight context:0];
}

- (void)replaceSubview:(id)aSubview withView:(id)aView withAttributes:(id)aAttributes
{
	NSUInteger index = [self indexOfSubview:aSubview];
	WGTitledView *titled = index != NSNotFound ? [[splitView subviews] objectAtIndex:index] : nil;
	[titled setContentView:aView attributes:aAttributes];
}

- (void)removeSubview:(id)aSubview
{
	[aSubview removeFromSuperview];
}

@end
