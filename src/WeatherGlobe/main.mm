// The application's main: LFoundation and the button cells' accessibility
// titles, then AppKit.

#import "WeatherGlobePrefix.h"
#import "WGButtonCell.h"
#include <objc/runtime.h>

static void finilizeAtlas()
{
	AppController::classObject()->killSingleton();
	finilizeLFoundation();
}

// NSCell's and NSView's awakeFromNib (NSObject's when they have none).
static IMP _cellAwakeFromNib = NULL;
static IMP _viewAwakeFromNib = NULL;

// The nibs are laid out for Lucida Grande, but AppKit now loads their Lucida
// Grande fonts as the system font, which is wider: labels got cut off
// ("Slow" and "Fast" in the time palette). The nibs have no other system
// font, so a system font from a nib goes back to Lucida Grande.
static NSFont *_lucidaFontForFont(NSFont *font)
{
	if (![[font fontName] hasPrefix:@".AppleSystemUI"])
		return nil;
	BOOL bold = ([[font fontDescriptor] symbolicTraits] & NSFontDescriptorTraitBold) != 0;
	return [NSFont fontWithName:bold ? @"LucidaGrande-Bold" : @"LucidaGrande" size:[font pointSize]];
}

static void _useLucidaInCell(NSCell *cell)
{
	if ([cell type] != NSTextCellType)
		return;
	NSFont *lucida = _lucidaFontForFont([cell font]);
	if (lucida)
		[cell setFont:lucida];
}

static void _wgCellAwakeFromNib(NSCell *self, SEL _cmd)
{
	((void (*)(id, SEL))_cellAwakeFromNib)(self, _cmd);
	_useLucidaInCell(self);
}

// Controls' cells get no awakeFromNib of their own.
static void _wgViewAwakeFromNib(NSView *self, SEL _cmd)
{
	((void (*)(id, SEL))_viewAwakeFromNib)(self, _cmd);
	if ([self isKindOfClass:[NSMatrix class]]) {
		for (NSCell *cell in [(NSMatrix *)self cells])
			_useLucidaInCell(cell);
	} else if ([self isKindOfClass:[NSTableView class]]) {
		for (NSTableColumn *column in [(NSTableView *)self tableColumns]) {
			_useLucidaInCell([column dataCell]);
			_useLucidaInCell([column headerCell]);
		}
	} else if ([self isKindOfClass:[NSControl class]]) {
		_useLucidaInCell([(NSControl *)self cell]);
	} else if ([self isKindOfClass:[NSBox class]]) {
		_useLucidaInCell([(NSBox *)self titleCell]);
	} else if ([self isKindOfClass:[NSTabView class]]) {
		NSFont *lucida = _lucidaFontForFont([(NSTabView *)self font]);
		if (lucida)
			[(NSTabView *)self setFont:lucida];
	}
}

static void _replaceAwakeFromNib(Class aClass, IMP aImp, IMP *aOld)
{
	Method method = class_getInstanceMethod(aClass, @selector(awakeFromNib));
	*aOld = method_getImplementation(method);
	if (!class_addMethod(aClass, @selector(awakeFromNib), aImp, method_getTypeEncoding(method)))
		*aOld = method_setImplementation(method, aImp);
}

// Every button cell uses WGButtonCell's accessibility title. The original
// made WGButtonCell pose as NSButtonCell, which the 64-bit runtime can't
// do; NSButtonCell's method is replaced instead. The nibs' fonts stay Lucida
// Grande.
static void initializeAtlas()
{
	atexit(finilizeAtlas);
	initializeLFoundation(true, true);
	[WGButtonCell installAsButtonCellAccessibility];
	_replaceAwakeFromNib([NSCell class], (IMP)_wgCellAwakeFromNib, &_cellAwakeFromNib);
	_replaceAwakeFromNib([NSView class], (IMP)_wgViewAwakeFromNib, &_viewAwakeFromNib);
}

int main(int argc, char *argv[])
{
	initializeAtlas();
	return NSApplicationMain(argc, (const char **)argv);
}
