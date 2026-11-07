// _bitmapRepGrabCallback: a grabbed back buffer as a 24-bit RGB bitmap.

#import "WeatherGlobePrefix.h"
#include "WGAppClasses.h"

void _bitmapRepGrabCallback::setOutSize(LSize size)
{
	_width = size.width > 0.0f ? (unsigned int)size.width : 0;
	_height = size.height > 0.0f ? (unsigned int)size.height : 0;
	[_rep release];
	_rep = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:0 pixelsWide:_width pixelsHigh:_height
	                                            bitsPerSample:8 samplesPerPixel:3 hasAlpha:NO isPlanar:NO
	                                           colorSpaceName:NSDeviceRGBColorSpace bytesPerRow:_width * 3
	                                             bitsPerPixel:24];
}

// The rows' 4-byte pixels without their last byte.
void _bitmapRepGrabCallback::dataIsReady(const void *bytes, unsigned int firstRow, unsigned int rowsCount,
                                         unsigned int rowBytes)
{
	unsigned char *data = [_rep bitmapData];
	if (!data)
		return;
	NSInteger bytesPerRow = [_rep bytesPerRow];
	const unsigned char *srcRow = (const unsigned char *)bytes;
	for (unsigned int row = 0; row < rowsCount; row++) {
		unsigned char *dst = data + (row + firstRow) * bytesPerRow;
		const unsigned char *src = srcRow;
		for (unsigned int x = 0; x < _width; x++) {
			dst[0] = src[0];
			dst[1] = src[1];
			dst[2] = src[2];
			dst += 3;
			src += 4;
		}
		srcRow += rowBytes;
	}
}
