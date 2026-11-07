#import "WGCGImageCell.h"

// The object value is a CGImage. It's released when the app quits (before
// CoreGraphics goes away).
@implementation WGCGImageCell

- (void)_appWillTerminate
{
	if (_cgImage)
		CFRelease(_cgImage);
	_cgImage = 0;
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	if (_cgImage)
		CFRelease(_cgImage);
	[super dealloc];
}

- (void)setObjectValue:(id)aValue
{
	CGImageRef image = (CGImageRef)aValue;
	if (_cgImage == image)
		return;
	if (_cgImage)
		CFRelease(_cgImage);
	_cgImage = image;
	if (image) {
		CFRetain(image);
		if (!_observerWasAdded) {
			[[NSNotificationCenter defaultCenter] addObserver:self
			                                         selector:@selector(_appWillTerminate)
			                                             name:NSApplicationWillTerminateNotification
			                                           object:nil];
			_observerWasAdded = YES;
		}
	}
}

- (id)objectValue
{
	return (id)_cgImage;
}

- (id)copyWithZone:(NSZone *)aZone
{
	WGCGImageCell *copy = [[self class] new];
	[copy setObjectValue:[self objectValue]];
	return copy;
}

// Scaled to fit an even-sided frame, centered, unsmoothed.
- (void)drawInteriorWithFrame:(NSRect)aFrame inView:(id)aView
{
	if (!_cgImage)
		return;
	float width = (float)CGImageGetWidth(_cgImage);
	float height = (float)CGImageGetHeight(_cgImage);
	int frameWidth = (int)aFrame.size.width, frameHeight = (int)aFrame.size.height;
	float scale = MIN((float)((frameWidth - 1) + ((frameWidth & 1) == 0)) / width,
	                  (float)((frameHeight - 1) + ((frameHeight & 1) == 0)) / height);
	float drawnHeight = (float)(unsigned)MAX(height * scale, 0.0f);
	float drawnWidth = (float)(unsigned)MAX(width * scale, 0.0f);
	float y = floorf((aFrame.size.height - drawnHeight) * 0.5 + aFrame.origin.y);
	float x = floorf((aFrame.size.width - drawnWidth) * 0.5 + aFrame.origin.x);
	BOOL flipped = [[NSView focusView] isFlipped];
	CGContextRef context = [[NSGraphicsContext currentContext] CGContext];
	CGContextSaveGState(context);
	CGContextSetInterpolationQuality(context, kCGInterpolationNone);
	if (flipped) {
		CGContextTranslateCTM(context, 0, y);
		CGContextConcatCTM(context, CGAffineTransformMake(1, 0, 0, -1, 0, drawnHeight));
		y = 0;
	}
	CGContextDrawImage(context, CGRectMake(x, y, drawnWidth, drawnHeight), _cgImage);
	CGContextRestoreGState(context);
}

@end
