#import "SMKGridTextFieldCell.h"

static SMKStringRenderer *sStringRenderer;

// A grid cell drawing its string, truncated to fit, in the grid's font.
@implementation SMKGridTextFieldCell

- (id)init
{
	if (sStringRenderer == nil) {
		sStringRenderer = [SMKStringRenderer new];
		[sStringRenderer setAlignment:NSTextAlignmentLeft];
	}
	return [super init];
}

- (void)drawInteriorWithFrame:(NSRect)aFrame inView:(id)aView
{
	NSString *string = [self stringValue];
	[sStringRenderer setFont:[[aView dataSource] textFont]];
	[sStringRenderer setContainerWidth:aFrame.size.width];
	NSString *fitted = [sStringRenderer stringByFittingString:string toSize:aFrame.size boundingRect:NULL];
	[sStringRenderer drawString:fitted atPoint:aFrame.origin withColor:[self isHighlighted] ? [NSColor whiteColor] : nil];
}

@end
