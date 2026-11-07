#import "WGButtonCell.h"
#include <objc/runtime.h>

// NSButtonCell's own accessibilityAttributeValue:.
static IMP _buttonCellAttributeValue = NULL;

// The shown title (the alternate one when on, if there is one) is the
// accessibility title.
static id _wgAttributeValue(NSButtonCell *self, SEL _cmd, id aValue)
{
	if ([aValue isEqualToString:NSAccessibilityTitleAttribute]) {
		NSString *title = [self state] == NSControlStateValueOn && [[self alternateTitle] length] ? [self alternateTitle]
		                                                                                         : [self title];
		if ([title length])
			return title;
	}
	return ((id (*)(id, SEL, id))_buttonCellAttributeValue)(self, _cmd, aValue);
}

@implementation WGButtonCell

+ (void)installAsButtonCellAccessibility
{
	if (_buttonCellAttributeValue)
		return;
	Method method = class_getInstanceMethod([NSButtonCell class], @selector(accessibilityAttributeValue:));
	_buttonCellAttributeValue = method_setImplementation(method, (IMP)_wgAttributeValue);
}

- (id)accessibilityAttributeValue:(id)aValue
{
	return _wgAttributeValue(self, _cmd, aValue);
}

@end
