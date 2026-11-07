// CPRFont: the system font at a size and a color, as the Cocoa string
// attributes of the printed overlays.

#import <Cocoa/Cocoa.h>
#include "RenderLib.h"

LF_IMPLEMENT_CLASS(CPRFont, 0)

const char *CPRFont::kFontDefaultSystemFont = "Arial";

void CPRFont::dealloc()
{
	releaseAndClear(_attributes);
	LObject::dealloc();
}

CPRFont *CPRFont::fontWithParams(float size, const char *name, unsigned int flags, const Color &color, void *reserved)
{
	CPRFont *font = CPRFont::_alloc()->initWithParams(size, name, flags, color, reserved);
	return font ? LAutorelease(font) : 0;
}

CPRFont *CPRFont::initWithParams(float size, const char *, unsigned int, const Color &color, void *)
{
	if (LObject::init() == 0)
		return 0;
	_size = size;
	NSFont *font = [NSFont systemFontOfSize:size];
	NSColor *nsColor = [NSColor colorWithDeviceRed:color.r green:color.g blue:color.b alpha:color.a];
	NSDictionary *attributes =
	    [NSDictionary dictionaryWithObjectsAndKeys:nsColor, NSForegroundColorAttributeName, font, NSFontAttributeName, nil];
	_attributes = LObjToNSObjWrapper::_alloc()->initWithNSObject(attributes);
	return this;
}

void CPRFont::drawString(LString *string, const LPoint &point, LSize *)
{
	[(NSString *)string->cfString() drawAtPoint:NSMakePoint(point.x, point.y)
	                              withAttributes:(NSDictionary *)_attributes->nsObject()];
}

LSize CPRFont::stringSize(LString *string)
{
	NSSize size = [(NSString *)string->cfString() sizeWithAttributes:(NSDictionary *)_attributes->nsObject()];
	return LMakeSize((float)size.width, (float)size.height);
}
