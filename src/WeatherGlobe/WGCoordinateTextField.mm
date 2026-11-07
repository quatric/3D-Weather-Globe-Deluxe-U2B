#import "WGNewLocationController.h"

@implementation WGCoordinateTextField

// Selects the text once the field editor is in place.
- (BOOL)becomeFirstResponder
{
	BOOL result = [super becomeFirstResponder];
	[self performSelector:@selector(selectText:) withObject:nil afterDelay:0.01];
	return result;
}

@end
