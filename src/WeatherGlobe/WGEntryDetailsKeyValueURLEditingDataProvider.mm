#import "WGEntryDetailsKeyValueURLEditingDataProvider.h"
#import "WGAutoAdjustableTextField.h"

// The keys of a key-value pair's dictionary.
static LConstantString _sKeyValueDictionaryKeyKey("key");
static LConstantString _sKeyValueDictionaryValueKey("value");
LString *_kKeyValueDictionaryKeyKey = &_sKeyValueDictionaryKeyKey;
LString *_kKeyValueDictionaryValueKey = &_sKeyValueDictionaryValueKey;

// The editable link: two rows, "Link" (the URL) and "Display" (its title),
// each a title beside a growing field; the views relayout as they resize.
@implementation WGEntryDetailsKeyValueURLEditingDataProvider

- (Class)descriptionViewClass
{
	return [WGAutoAdjustableTextField class];
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	[super dealloc];
}

- (BOOL)isDescriptionEditable
{
	return YES;
}

- (BOOL)tryStartDescriptionEditing
{
	return [[_descriptionView window] makeFirstResponder:_linkDescription];
}

- (id)titleView
{
	if (!_titleView) {
		_linkTitle = [self createTitleViewWithTitleString:[[NSBundle mainBundle] localizedStringForKey:@"link" value:@"" table:nil]];
		_titleTitle = [self createTitleViewWithTitleString:[[NSBundle mainBundle] localizedStringForKey:@"display" value:@"" table:nil]];
		_titleView = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 100.0, 100.0)];
		[_titleView setPostsFrameChangedNotifications:YES];
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(rebuildLayout)
			name:NSViewFrameDidChangeNotification object:_titleView];
		[_titleView addSubview:_linkTitle];
		[_titleView addSubview:_titleTitle];
		[_linkTitle release];
		[_titleTitle release];
	}
	return _titleView;
}

- (void)rebuildLayout:(id)aLayout
{
	[self rebuildLayout];
}

// The title shows the placeholder only while both fields are empty.
- (void)updateDescription
{
	NSString *title = [self urlTitle];
	NSString *url = [self urlValue];
	[_linkDescription setStringValue:url];
	[_titleDescription setStringValue:title];
	[[_titleDescription cell] setPlaceholderString:![title length] && [url length] ? url : [self descriptionPlaceholder]];
}

- (void)validateDataObject
{
	[self validateDataObject:nil];
}

// While the title is empty, its placeholder follows the URL.
- (void)controlTextDidChange:(id)aChange
{
	if ([aChange object] != _linkDescription)
		return;
	NSString *title = [_titleDescription stringValue];
	NSString *url = [_linkDescription stringValue];
	[[_titleDescription cell] setPlaceholderString:![title length] && [url length] ? url : [self descriptionPlaceholder]];
}

- (void)controlTextDidEndEditing:(id)aEditing
{
	[self validateDataObject:[aEditing object]];
}

- (id)objectValueForControl:(id)aControl
{
	if ([aControl superview] != _descriptionView)
		return [super objectValueForControl:aControl];
	if (aControl == _linkDescription)
		return [self urlValue];
	if (aControl == _titleDescription)
		return [self urlTitle];
	return nil;
}

// Both rows are as tall as the two fields, or the title cells, together.
// (The original measured the link title cell twice.)
- (void)viewSizesForSuperviewBounds:(NSRect)aBounds titleWidth:(CGFloat)aWidth offset:(CGFloat)aOffset outTitleSize:(NSSize *)aSize outDescriptionSize:(NSSize *)aSize2
{
	CGFloat width = aOffset * -2.0 + (aBounds.size.width - aWidth);
	CGFloat fieldHeight = WGEntryDetailsInfoDefaultFieldSize.height;
	NSSize link = [_linkDescription sizeForProposedSize:NSMakeSize(width, fieldHeight)];
	NSSize title = [_titleDescription sizeForProposedSize:NSMakeSize(width, fieldHeight)];
	CGFloat height = aOffset + title.height + link.height;
	NSRect titleBounds = NSMakeRect(NSZeroPoint.x, NSZeroPoint.y, aWidth, fieldHeight);
	NSSize linkTitle = [[_linkTitle cell] cellSizeForBounds:titleBounds];
	NSSize titleTitle = [[_linkTitle cell] cellSizeForBounds:titleBounds];
	CGFloat titlesHeight = aOffset + titleTitle.height + linkTitle.height;
	if (height <= titlesHeight)
		height = titlesHeight;
	if (aSize2)
		*aSize2 = NSMakeSize(width, height);
	if (aSize)
		*aSize = NSMakeSize(aWidth, height);
}

- (id)descriptionView
{
	if (_descriptionView)
		return _descriptionView;
	NSString *title = [self urlTitle];
	NSString *url = [self urlValue];
	_linkDescription = [self createDescriptionViewWithDescription:url];
	_titleDescription = [self createDescriptionViewWithDescription:title];
	[[_titleDescription cell] setPlaceholderString:![title length] && [url length] ? url : [self descriptionPlaceholder]];
	[_linkDescription setOwner:self];
	[_linkDescription setDelegate:(id)self];
	[_titleDescription setOwner:self];
	[_titleDescription setDelegate:(id)self];
	_descriptionView = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 100.0, 100.0)];
	[_descriptionView setPostsFrameChangedNotifications:YES];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(rebuildLayout)
		name:NSViewFrameDidChangeNotification object:_descriptionView];
	[_descriptionView addSubview:_linkDescription];
	[_descriptionView addSubview:_titleDescription];
	[_linkDescription release];
	[_titleDescription release];
	return _descriptionView;
}

// Stacks the "Display" row over the "Link" row (4 apart), titles aligned
// to the bottom of each row; the owner relayouts when the height changes.
- (void)rebuildLayout
{
	if (!_descriptionView || !_titleView)
		return;
	NSRect bounds = [_descriptionView bounds];
	NSRect titleBounds = [_titleView bounds];
	NSRect cellBounds = NSMakeRect(NSZeroPoint.x, NSZeroPoint.y, titleBounds.size.width,
		WGEntryDetailsInfoDefaultFieldSize.height);
	CGFloat y = bounds.origin.y;

	CGFloat titleHeight = [[_titleTitle cell] cellSizeForBounds:cellBounds].height;
	CGFloat fieldHeight = [_titleDescription sizeForProposedSize:bounds.size].height;
	CGFloat row = titleHeight <= fieldHeight ? fieldHeight : titleHeight;
	[_titleTitle setFrame:NSMakeRect(titleBounds.origin.x, row + y - titleHeight, titleBounds.size.width, titleHeight)];
	[_titleDescription setFrame:NSMakeRect(bounds.origin.x, y, bounds.size.width, row)];
	y = row + 4.0 + y;

	titleHeight = [[_linkTitle cell] cellSizeForBounds:cellBounds].height;
	fieldHeight = [_linkDescription sizeForProposedSize:bounds.size].height;
	row = titleHeight <= fieldHeight ? fieldHeight : titleHeight;
	[_linkTitle setFrame:NSMakeRect(titleBounds.origin.x, y + row - titleHeight, titleBounds.size.width, titleHeight)];
	[_linkDescription setFrame:NSMakeRect(bounds.origin.x, y, bounds.size.width, row)];

	CGFloat bottom = NSMaxY([_linkDescription frame]);
	if (bottom < NSMaxY([_linkTitle frame]))
		bottom = NSMaxY([_linkTitle frame]);
	if (bottom != [_descriptionView bounds].size.height)
		[[self owner] rebuildLayout];
}

// Rebuilds the URL map from the edited field (or both, for nil) when it
// changed; no URL clears it.
- (void)validateDataObject:(id)aObject
{
	NSString *url = [self urlValue];
	NSString *title = [self urlTitle];
	BOOL changed = NO;
	if (!aObject || aObject == _linkDescription) {
		NSString *string = [_linkDescription stringValue];
		changed = ![string isEqual:url];
		if (changed)
			url = string;
	}
	if (!aObject || aObject == _titleDescription) {
		NSString *string = [_titleDescription stringValue];
		changed = ![string isEqual:title] || changed;
		title = string;
	}
	if (!changed)
		return;
	LString *urlString = [url LString];
	LString *titleString = [title LString];
	if (_object) {
		_object->release();
		_object = 0;
	}
	if (url) {
		LMap *map = LMap::_alloc()->init();
		if (urlString) {
			map->setObjectForKey(urlString, kEntryInfoItemURLKey);
			if (titleString)
				map->setObjectForKey(titleString, kEntryInfoItemTitleKey);
		}
		_object = map;
	}
	[super validateDataObject];
}

@end
