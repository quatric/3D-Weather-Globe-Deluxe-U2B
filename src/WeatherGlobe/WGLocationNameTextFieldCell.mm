#import "WGNewLocationController.h"

@implementation WGLocationNameTextFieldCell

// A copy of the cell whose placeholder is drawn in its text color at a
// third of the opacity.
- (id)initWithCell:(id)aCell
{
	self = [super init];
	if (self) {
		[self setEditable:[aCell isEditable]];
		[self setSelectable:[aCell isSelectable]];
		[self setFont:[aCell font]];
		[self setTextColor:[aCell textColor]];
		[self setDrawsBackground:[aCell drawsBackground]];
		[self setBackgroundColor:[aCell backgroundColor]];
		[self setAction:[aCell action]];
		[self setTarget:[aCell target]];
		NSColor *color = [NSColor colorWithCalibratedRed:[[self textColor] redComponent] green:[[self textColor] greenComponent]
		                                            blue:[[self textColor] blueComponent] alpha:0.333];
		NSDictionary *attributes = [[[NSDictionary alloc] initWithObjectsAndKeys:color, NSForegroundColorAttributeName,
		                                                                         [self font], NSFontAttributeName, nil] autorelease];
		_placeholderString = [[NSAttributedString alloc] initWithString:[aCell placeholderString] attributes:attributes];
	}
	return self;
}

- (void)dealloc
{
	[_placeholderString release];
	[super dealloc];
}

- (void)drawInteriorWithFrame:(NSRect)aFrame inView:(id)aView
{
	[super drawInteriorWithFrame:aFrame inView:aView];
	if (![self showsFirstResponder] && [[self stringValue] length] == 0)
		[_placeholderString drawInRect:aFrame];
}

@end
