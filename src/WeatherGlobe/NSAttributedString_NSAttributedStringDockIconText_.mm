#import "AtlasController.h"

// The text attachment showing a named image.
static NSAttributedString *_imageString(NSString *name)
{
	NSImage *image = [NSImage imageNamed:name];
	if (!image)
		return nil;
	NSFileWrapper *wrapper = [[NSFileWrapper new] autorelease];
	[wrapper setIcon:image];
	NSTextAttachment *attachment = [[[NSTextAttachment alloc] initWithFileWrapper:wrapper] autorelease];
	return [NSAttributedString attributedStringWithAttachment:attachment];
}

@implementation NSAttributedString (NSAttributedStringDockIconText)

// The Dock icon's temperature: a sign image, the rounded degrees and a
// degrees image, in white bold Arial.
+ (id)stringForTemperature:(CGFloat)aTemperature inCelsius:(BOOL)aCelsius
{
	float temperature = aTemperature;
	BOOL negative = temperature < 0.0f;
	if (negative)
		temperature = -temperature;
	NSMutableParagraphStyle *style = [[[NSParagraphStyle defaultParagraphStyle] mutableCopy] autorelease];
	[style setAlignment:NSTextAlignmentCenter];
	NSFont *font = [[NSFontManager sharedFontManager] fontWithFamily:@"Arial" traits:NSBoldFontMask weight:9 size:36.0];
	if (!font)
		font = [NSFont boldSystemFontOfSize:30.0];
	NSDictionary *attributes = [NSDictionary dictionaryWithObjectsAndKeys:font, NSFontAttributeName,
	                                                                       [NSColor whiteColor], NSForegroundColorAttributeName,
	                                                                       style, NSParagraphStyleAttributeName, nil];
	NSMutableAttributedString *result = [[NSMutableAttributedString new] autorelease];
	NSAttributedString *part = _imageString(negative ? @"sign_minus.tiff" : @"sign_plus.tiff");
	if (part)
		[result appendAttributedString:part];
	NSString *degrees = [NSString stringWithFormat:@"%d", (int)(temperature + 0.5f)];
	[result appendAttributedString:[[[NSMutableAttributedString alloc] initWithString:degrees attributes:nil] autorelease]];
	part = _imageString(aCelsius ? @"temper_C.tiff" : @"temper_F.tiff");
	if (part)
		[result appendAttributedString:part];
	[result addAttributes:attributes range:NSMakeRange(0, [result length])];
	return result;
}

@end
