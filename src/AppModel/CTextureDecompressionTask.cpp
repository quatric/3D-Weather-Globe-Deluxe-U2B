// CTextureDecompressionTask: reads an image file (JPEG, PNG or DDS) into a
// bitmap of a texture format, in the background or at once.

#include "AppModel.h"

LF_IMPLEMENT_CLASS(CTextureDecompressionTask, LF_CLASSOF(LTask))

void CTextureDecompressionTask::dealloc()
{
	if (_results) {
		_results->release();
		_results = 0;
	}
	if (_path) {
		_path->release();
		_path = 0;
	}
	LTask::dealloc();
}

void CTextureDecompressionTask::cleanup()
{
	if (_target) {
		_target->release();
		_target = 0;
	}
}

// A format of 0 is the image's own.
CTextureDecompressionTask *CTextureDecompressionTask::initWithParams(LString *path, D3DFORMAT format, LObject *target,
                                                                     const char *sel, void *context)
{
	_results = 0;
	if (LTask::initWithTargetAndSelector(target, sel) == 0)
		return 0;
	_context = context;
	_path = LString::_alloc()->init();
	_path->setValue(path);
	_width = 0;
	_height = 0;
	_format = format;
	return this;
}

void CTextureDecompressionTask::execute()
{
	_results = LArray::_alloc()->init();
	LData *data = LData::dataWithContentsOfFile(_path);
	LF_CHECK_PTR(data);
	ImageDecompressor *decompressor = ImageDecompressor::decompressorForData(data);
	LF_CHECK_PTR(decompressor);
	if (_format == 0)
		_format = decompressor->nativeFormat();
	_width = decompressor->width();
	_height = decompressor->height();
	LData *bitmap = LData::dataWithLength(FHGetSurfaceSize(_width, _height, _format));
	if (!decompressor->decompressToBitmapWithFormat(_format, bitmap->bytes()))
		LF_RAISE(WrongConditionException());
	_results->addObject(bitmap);
	if (_results->count() == 0 && _results) {
		_results->release();
		_results = 0;
	}
}
