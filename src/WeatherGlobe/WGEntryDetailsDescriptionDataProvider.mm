#import "WGEntryDetailsDescriptionDataProvider.h"
#import "WGAutoAdjustableTextField.h"

// The entry's description: a growing text field, editable while the owner
// edits; "Title. text" shows the title in bold.
@implementation WGEntryDetailsDescriptionDataProvider

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	[super dealloc];
}

- (Class)descriptionViewClass
{
	return [WGAutoAdjustableTextField class];
}

- (id)title
{
	return nil;
}

- (id)descriptionView
{
	if (!_descriptionView) {
		_descriptionView = [self createDescriptionViewWithDescription:@""];
		[self updateDescription];
		[(WGAutoAdjustableTextField *)_descriptionView setOwner:self];
		[(WGAutoAdjustableTextField *)_descriptionView setDelegate:(id)self];
	}
	return _descriptionView;
}

- (id)descriptionPlaceholder
{
	if (![[self owner] isEditing])
		return @"";
	return [[NSBundle mainBundle] localizedStringForKey:@"Description" value:@"" table:nil];
}

- (BOOL)isDescriptionEditable
{
	return [[self owner] isEditing];
}

- (void)rebuildLayout:(id)aLayout
{
	[self rebuildLayout];
}

- (void)rebuildLayout
{
	[[self owner] rebuildLayout];
}

- (void)controlTextDidEndEditing:(id)aEditing
{
	if ([[self owner] isEditing]) {
		[super controlTextDidEndEditing:aEditing];
		[self validateDataObject];
	}
}

- (id)description
{
	return [NSString stringReferenceFromLString:lobj_cast<LString>(_object)];
}

// The field's width beside the title (if any), its height for that width.
- (NSSize)descriptionSizeForSuperviewBounds:(NSRect)aBounds titleWidth:(CGFloat)aWidth offset:(CGFloat)aOffset
{
	CGFloat width = [self title] ? aBounds.size.width - aWidth - aOffset * 2.0
		: aBounds.size.width - aOffset * 2.0;
	NSSize size = [(WGAutoAdjustableTextField *)_descriptionView sizeForProposedSize:
		NSMakeSize(width, WGEntryDetailsInfoDefaultFieldSize.height)];
	return NSMakeSize(width, size.height);
}

- (void)updateDescription
{
	LString *title = 0, *text = 0;
	EntryInfoController::extractDescriptionParamsFromObject(_object, &title, &text);
	NSString *string = text ? (NSString *)text->cfString() : @"";
	NSString *titleString = title ? (NSString *)title->cfString() : nil;
	if (!titleString) {
		[(NSTextField *)_descriptionView setStringValue:string];
		return;
	}
	[(NSTextField *)_descriptionView setStringValue:[titleString stringByAppendingFormat:@". %@", string]];
	NSMutableAttributedString *attributed = [[[(NSTextField *)_descriptionView attributedStringValue]
		mutableCopy] autorelease];
	[attributed addAttribute:NSFontAttributeName value:[NSFont boldSystemFontOfSize:11.0]
		range:NSMakeRange(0, [titleString length] + 2)];
	[(NSTextField *)_descriptionView setAttributedStringValue:attributed];
}

- (LObject *)dataObject
{
	NSString *string = [(NSTextField *)_descriptionView stringValue];
	if ([string length])
		return LString::stringWithCFString((CFStringRef)string);
	return 0;
}

@end
