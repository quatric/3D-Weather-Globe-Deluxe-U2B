// ImagesSequence: the weather and wind icons. Each file is a row of square
// icons, each icon a column of frames; the frames of an icon are cut out on
// first use and kept.

#include "AppModel.h"

LF_IMPLEMENT_CLASS(ImagesSequence, 0)

void ImagesSequence::dealloc()
{
	if (_images) {
		_images->release();
		_images = 0;
	}
	if (_cache) {
		_cache->release();
		_cache = 0;
	}
	LObject::dealloc();
}

LBitmapImage *ImagesSequence::_createImageForIconAtIndex(int index, int frame)
{
	LBitmapImage *source = (LBitmapImage *)_images->objectAtIndex(index / _iconsPerRow);
	LBitmapImage *image = LBitmapImage::_alloc()->initWithSizeFormat(_iconSize, _iconSize, kPixelFormatBGRA);
	LBitmapImage::StLocker src(source, LBitmapImage::StLocker::kRead, kPixelFormatBGRA);
	LBitmapImage::StLocker dst(image, LBitmapImage::StLocker::kWrite, kPixelFormatBGRA);
	unsigned int srcRow = src.bytesPerRow();
	unsigned int dstRow = dst.bytesPerRow();
	const char *from = (const char *)src.bitmapData() + (index % _iconsPerRow) * (src.bitsPerPixel() >> 3) * _iconSize +
	                   _iconSize * srcRow * frame;
	char *to = (char *)dst.bitmapData();
	for (int i = 0; i < _iconSize; i++) {
		memcpy(to, from, srcRow <= dstRow ? srcRow : dstRow);
		to += dstRow;
		from += srcRow;
	}
	return image;
}

ImagesSequence *ImagesSequence::initWithFiles(TLArray<LString *> *files, int iconSize)
{
	if (LObject::init() == 0)
		return 0;
	int count = files ? files->count() : 0;
	_iconSize = iconSize;
	for (int i = 0; i < count; i++) {
		LBitmapImage *image = LBitmapImage::_alloc()->initWithContentsOfFile(files->objectAtIndex(i));
		if (image == 0)
			continue;
		LSize size = image->size();
		if (_images == 0) {
			_images = LArray::_alloc()->init();
			_iconsPerRow = (int)size.width / _iconSize;
			_framesCount = (int)size.height / _iconSize;
		}
		_iconsCount += (int)size.width / _iconSize;
		_images->addObject(image);
		image->release();
	}
	return this;
}

LArray *ImagesSequence::imagesForIconAtIndex(int index)
{
	if (index < 1 || _iconsCount < index) {
		LLog("Wrong index passed for icon lookup.\n");
		index = _iconsCount;
	}
	LString *key = LString::stringWithInt(index);
	if (_cache) {
		LArray *frames = (LArray *)_cache->objectForKey(key);
		if (frames)
			return frames;
	} else {
		_cache = LMap::_alloc()->init();
	}
	LArray *frames = LArray::_alloc()->init();
	_cache->setObjectForKey(frames, key);
	frames->release();
	for (int i = 0; i < _framesCount; i++) {
		LBitmapImage *image = _createImageForIconAtIndex(index - 1, i);
		frames->addObject(image);
		image->release();
	}
	return frames;
}
