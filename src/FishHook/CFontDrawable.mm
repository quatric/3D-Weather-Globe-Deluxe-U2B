// CFontDrawable and CFontDrawableData: text drawn from a texture of the
// characters 0x20-0xff, painted with AppKit.

#import <AppKit/AppKit.h>
#include "FHSprites.h"
#import "NSString_LStringExtension.h"

LF_IMPLEMENT_CLASS(CFontDrawable, LF_CLASSOF(CBuffersOwner))
LF_IMPLEMENT_CLASS(CFontDrawableData, LF_CLASSOF(CDrawableModelData))

static LConstantString _kSharedFontsIB("AllFontsSharedIB");
static LConstantString _kElipsesString("...");

enum { kCharsPerDraw = 0x400 };

// The result _paintAlphabet gives when the characters don't fit.
#define FH_E_ALPHABET_TOO_BIG ((HRESULT)0x88760867)

// ---------------------------------------------------------------------------
// _textIterator

_textIterator::_textIterator(const LString *string) : _string(LRetain(const_cast<LString *>(string))), _newLine(false)
{
	_p = _string ? _string->chars() : 0;
}

bool _textIterator::nextChar(UniChar *c)
{
	_newLine = false;
	if (_p == 0 || *_p == 0)
		return false;
	UniChar ch = *_p++;
	if (ch == '\n') {
		_newLine = true;
		return false;
	}
	if (ch == '\r' && *_p == '\n') {
		_p++;
		_newLine = true;
		return false;
	}
	*c = ch;
	return true;
}

int _textIterator::currentOffset() const
{
	if (_string == 0)
		return 0;
	return (int)(_p - _string->chars()) - (*_p != 0 ? 1 : 0);
}

// ---------------------------------------------------------------------------
// CFontDrawable

CFontDrawable::CFontDrawable()
    : _fontFlags(0), _fontName(0), _fontSize(0), _ellipsisWidth(0), _vb(0), _positionOffset(0), _colorOffset(0),
      _texcoordOffset(0), _vertexSize(0), _count(-1), _rendering(false), _locked(0), _color(0), _texture(0),
      _firstChar(0x20), _lastChar(0xff), _charCoords(0), _scaleX(256), _scaleY(256), _charSpacing(0), _fontScale(1),
      _textureWidth(0x100), _textureHeight(0x100), _bitmap(0), _lockedImage(0), _userSpaceScale(1),
      _oldSystem(false)
{
}

CFontDrawable *CFontDrawable::initWithHolderAndParameters(CHolder *holder, LString *fontName, unsigned int size,
                                                          unsigned int flags)
{
	CRenderProfile *profile = CRenderProfile::renderProfileByFileDesc(&FontRenderProfile, holder);
	if (initWithDataSource(this, profile, 0) == 0)
		return 0;
	_fontSize = size;
	_fontFlags = flags;
	_fontName = LRetain(fontName);
	unsigned int count = (_lastChar - _firstChar + 1) * 4;
	_charCoords = new float[count];
	memset(_charCoords, 0, count * sizeof(float));
	if (holder)
		holder->beginAccumulationBlock();
	_texture = C2DTexture::_alloc()->initWithDataSource(this, holder, 0);
	if (holder)
		holder->endAccumulationBlock();
	setTextureForSource(_texture, 0);
	setTextureForSource(_texture, 1);
	return this;
}

void CFontDrawable::dealloc()
{
	LRelease(_fontName);
	_fontName = 0;
	if (_vb) {
		_vb->dataSourceWillDie();
		_vb->release();
		_vb = 0;
	}
	if (_texture) {
		_texture->dataSourceWillDie();
		_texture->release();
		_texture = 0;
	}
	delete[] _charCoords;
	_charCoords = 0;
	if (_bitmap) {
		_deleteOffscreenImage(_bitmap);
		_bitmap = 0;
	}
	CBuffersOwner::dealloc();
}

void CFontDrawable::instantiateSelf()
{
	CBuffersOwner::instantiateSelf();
	CRenderProfile *profile = renderProfile();
	_positionOffset = profile->drawableElementOffset(D3DDECLUSAGE_POSITION, 0, 0);
	_colorOffset = profile->drawableElementOffset(D3DDECLUSAGE_COLOR, 0, 0);
	_texcoordOffset = profile->drawableElementOffset(D3DDECLUSAGE_TEXCOORD, 0, 0);
	_ellipsisWidth = 0;
	_vertexSize = profile->vertexStructSize(0);
}

LClass *CFontDrawable::modelDataClass()
{
	return LF_CLASSOF(CFontDrawableData);
}

unsigned int CFontDrawable::widthOfTexture(C2DTexture *, CContext *)
{
	if (!_charsBitmapIsCreated())
		_createCharsBitmap();
	return _textureWidth;
}

unsigned int CFontDrawable::heightOfTexture(C2DTexture *, CContext *)
{
	if (!_charsBitmapIsCreated())
		_createCharsBitmap();
	return _textureHeight;
}

void CFontDrawable::textureWasInstantiated(CBaseTexture *)
{
	if (!_charsBitmapIsCreated())
		_createCharsBitmap();
	_copyCharsBitmapToTexture();
	_deleteCharsBitmap();
}

void CFontDrawable::_startTextRendering()
{
	if (!_rendering) {
		_count = -1;
		_rendering = true;
	}
}

void CFontDrawable::_finishTextRendering()
{
	if (!_rendering)
		return;
	if (_count >= 0 && _locked) {
		vertexBufferForBufOwner(0, this)->_vb->Unlock();
		_locked = 0;
		_context->_drawCache->drawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, _count * 4, 0, _count * 2);
		_count = -1;
	}
	_rendering = false;
}

void CFontDrawable::_deleteCharsBitmap()
{
	if (_bitmap)
		_deleteOffscreenImage(_bitmap);
	_bitmap = 0;
}

// Paints the characters into a texture of 128x128 or more, shrinking the
// font when it needs a texture bigger than the device takes.
void CFontDrawable::_createCharsBitmap()
{
	_fontScale = 1;
	_textureWidth = _textureHeight = 0x80;
	_userSpaceScale = _holder->_view->userSpaceScaleFactor();
	void *image = _createOffscreenImage();
	void *font = _createGDIFont();
	if (font) {
		HRESULT hr;
		while ((hr = _paintAlphabet(image, font, 1)) == FH_E_ALPHABET_TOO_BIG) {
			_textureWidth <<= 1;
			_textureHeight <<= 1;
		}
		if (hr >= 0) {
			unsigned int maxSize = _holder->_context->_caps->_maxTextureSize;
			if (maxSize < _textureWidth) {
				_fontScale = (float)maxSize / (float)_textureWidth;
				_textureWidth = _textureHeight = maxSize;
				for (;;) {
					_deleteGDIFont(font);
					font = _createGDIFont();
					if (_paintAlphabet(image, font, 1) != FH_E_ALPHABET_TOO_BIG)
						break;
					_fontScale *= 0.9f;
				}
			}
			if (image)
				_deleteOffscreenImage(image);
			if (_bitmap)
				_deleteOffscreenImage(_bitmap);
			_bitmap = _createOffscreenImage();
			_paintAlphabet(_bitmap, font, 0);
			image = 0;
		}
	}
	if (image)
		_deleteOffscreenImage(image);
	if (font)
		_deleteGDIFont(font);
}

void CFontDrawable::_copyCharsBitmapToTexture()
{
	C2DTexture::StLocker locker(_texture, 0, D3DLOCK_DISCARD);
	const uint32_t *src = (const uint32_t *)_lockCharsImage();
	char *dst = (char *)locker.bits();
	for (unsigned int row = 0; row < _textureHeight; row++) {
		if (src && dst)
			memcpy(dst, src, _textureWidth * 4);
		dst += locker.bytesPerRow();
		src += _textureWidth;
	}
	_unlockCharsImage();
}

void CFontDrawable::fillBuffer(CIndexBuffer *ib)
{
	FHFillQuadIndices(ib, kCharsPerDraw);
}

CVertexBuffer *CFontDrawable::vertexBufferForBufOwner(int, CBuffersOwner *)
{
	if (_vb == 0) {
		CHolder *holder = _holder;
		if (holder)
			holder->beginAccumulationBlock();
		_vb = CVertexBuffer::_alloc()->initWithDataSource(this, holder, 0, 1);
		if (holder)
			holder->endAccumulationBlock();
	}
	return _vb;
}

// One index buffer serves all the fonts of a holder.
CIndexBuffer *CFontDrawable::indexBufferForBufOwner(CBuffersOwner *)
{
	CHolder *holder = _holder;
	LObject *shared = holder->objectForKey(&_kSharedFontsIB);
	if (shared && shared->getClass()->isSubclassOf(LF_CLASSOF(CIndexBuffer)))
		return (CIndexBuffer *)shared;
	if (holder)
		holder->beginAccumulationBlock();
	CIndexBuffer *ib = CIndexBuffer::_alloc()->initWithDataSource(this, primitiveTypeForBufOwner(this), holder, 0, 1);
	holder->setObjectForKey(ib, &_kSharedFontsIB);
	ib->release();
	if (holder)
		holder->endAccumulationBlock();
	return ib;
}

LSize CFontDrawable::calcTextSize(const LString *text)
{
	LSize size = LZeroSize;
	if (text == 0)
		return size;
	float lineHeight = _charsHeight();
	float width = 0;
	size.height = lineHeight;
	_textIterator it(text);
	while (it.hasMoreLines()) {
		UniChar c;
		while (it.nextChar(&c))
			if (_hasChar(c))
				width += _charWidth(c);
		if (width > size.width)
			size.width = width;
		if (it._newLine) {
			size.height += lineHeight;
			width = 0;
		}
	}
	return size;
}

// The start of each line, aligned in the rect by the flags; long lines are
// cut with "..." in the text itself when asked.
bool CFontDrawable::_evalLineOffsets(LString *text, const LRect &rect, unsigned int flags, float lineSpacing,
                                     TArray<Vector2> *offsets)
{
	if ((flags & 0x7f) == 0)
		return false;
	float y = LZeroPoint.y;
	float lineHeight = _charsHeight();
	bool ellipsize = (flags & kTextEllipsize) != 0;
	if (ellipsize && _ellipsisWidth == 0)
		_ellipsisWidth = calcTextSize(&_kElipsesString).width;
	_textIterator it(text);
	LString *copy = 0;
	int delta = 0;
	while (it.hasMoreLines()) {
		bool overflow = false;
		int cut = -1;
		float cutWidth = 0;
		float width = 0;
		UniChar c;
		while (it.nextChar(&c)) {
			if (!_hasChar(c))
				continue;
			float w = width + _charWidth(c);
			if (ellipsize && rect.size.width - _ellipsisWidth < w) {
				if (w <= rect.size.width) {
					if (cut < 0) {
						cut = it.currentOffset();
						cutWidth = width + _ellipsisWidth;
					}
				} else {
					overflow = true;
				}
			}
			width = w;
		}
		if (overflow) {
			if (copy == 0)
				copy = LString::stringWithString(text);
			int end = it.currentOffset();
			copy->replaceCharactersInRange(cut - delta, end - cut, &_kElipsesString);
			delta += (end - cut) - (int)_kElipsesString.length();
			width = cutWidth;
		}
		offsets->addObject(Vector2(width, y));
		y += lineHeight + lineSpacing;
	}
	float x0 = rect.origin.x, y0 = rect.origin.y;
	float w = rect.size.width, h = rect.size.height;
	CRenderTarget *target = _holder->_context->_currentRenderTarget;
	if (flags & kTextFullWidth) {
		x0 = 0;
		w = (float)target->_width;
	}
	if (flags & kTextFullHeight) {
		y0 = 0;
		h = (float)target->_height;
	}
	float hAlign = 0;
	if (flags & (kTextFullWidth | kTextCenterH))
		hAlign = 0.5f;
	if (flags & kTextRight)
		hAlign = 1;
	float vOffset = 0;
	if (flags & (kTextFullHeight | kTextCenterV))
		vOffset = (h - y) * 0.5f;
	int count = offsets->count();
	for (int i = 0; i < count; i++) {
		Vector2 &o = (*offsets)[i];
		o.x = (w - o.x) * hAlign + x0;
		o.y = y0 + o.y + vOffset;
	}
	if (copy)
		text->setValue(copy);
	return true;
}

// Writes a character's quad to the locked buffer, drawing when it is full.
void CFontDrawable::_renderChar(UniChar ch, const LRect *r)
{
	if (!_rendering)
		return;
	if (_count < 0) {
		vertexBufferForBufOwner(0, this)->_vb->Lock(0, 0, (void **)&_locked, D3DLOCK_DISCARD);
		_count = 0;
	}
	int first = _count * 4;
	char *base = _locked;
	float x[4] = {r->origin.x, r->origin.x + r->size.width, r->origin.x, r->origin.x + r->size.width};
	float y[4] = {r->origin.y, r->origin.y, r->origin.y - r->size.height, r->origin.y - r->size.height};
	for (int k = 0; k < 4; k++) {
		float *p = (float *)(base + (first + k) * _vertexSize + _positionOffset);
		p[0] = x[k];
		p[1] = y[k];
	}
	if (_texcoordOffset >= 0) {
		const float *c = _charCoords + (ch - _firstChar) * 4;
		float u[4] = {c[0], c[2], c[0], c[2]};
		float v[4] = {c[1], c[1], c[3], c[3]};
		for (int k = 0; k < 4; k++) {
			float *t = (float *)(base + (first + k) * _vertexSize + _texcoordOffset);
			t[0] = u[k];
			t[1] = v[k];
		}
	}
	if (_colorOffset >= 0)
		for (int k = 0; k < 4; k++)
			*(unsigned int *)(base + (first + k) * _vertexSize + _colorOffset) = _color;
	if (++_count >= kCharsPerDraw) {
		_count = -1;
		vertexBufferForBufOwner(0, this)->_vb->Unlock();
		_context->_drawCache->drawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, kCharsPerDraw * 4, 0, kCharsPerDraw * 2);
	}
}

void CFontDrawable::_renderText(LString *text, const LRect &rect, const LSize &, unsigned int color,
                                unsigned int flags, float lineSpacing)
{
	if (text == 0)
		return;
	_color = color;
	TArray<Vector2> offsets;
	bool aligned = _evalLineOffsets(text, rect, flags, lineSpacing, &offsets);
	float lineHeight = _charsHeight();
	_textIterator it(text);
	LSize virtualSize = _holder->virtualFrameSize();
	LSize relativeSize = _holder->relativeFrameSize();
	LRect charRect;
	charRect.size.width = 0;
	charRect.size.height = (2.0f / relativeSize.height) * lineHeight;
	unsigned int line = 0;
	while (it.hasMoreLines()) {
		float x = aligned ? offsets[line].x : rect.origin.x;
		charRect.origin.x = x * (2.0f / virtualSize.width) - 1;
		float y = aligned ? offsets[line].y : (float)line * (lineSpacing + lineHeight) + rect.origin.y;
		charRect.origin.y = 1 - y * (2.0f / virtualSize.height);
		UniChar c;
		while (it.nextChar(&c)) {
			if (!_hasChar(c))
				continue;
			charRect.size.width = _charWidth(c) * (2.0f / relativeSize.width);
			_renderChar(c, &charRect);
			charRect.origin.x += charRect.size.width;
		}
		line++;
	}
}

void CFontDrawable::drawSubset(CDrawableModelData *data, long, long)
{
	if (data == 0 || !data->getClass()->isSubclassOf(LF_CLASSOF(CFontDrawableData)))
		return;
	CFontDrawableData *fontData = (CFontDrawableData *)data;
	_startTextRendering();
	int count = fontData->_strings ? fontData->_strings->count() : 0;
	for (int i = 0; i < count; i++) {
		CFontDrawableData::SStringEntry *entry = (*fontData->_strings)[i];
		LString *text = LString::stringWithString(entry->_string);
		LSize scale = {1, 1};
		if (text == 0)
			continue;
		if (entry->_type == 0) {
			LRect rect;
			rect.origin = entry->_rect.origin;
			rect.size = LZeroRect.size;
			_renderText(text, rect, scale, entry->_color, fontData->_flags & 0xffffff83, fontData->_lineSpacing);
		} else if (entry->_type == 1) {
			_renderText(text, entry->_rect, scale, entry->_color, fontData->_flags & ~3u, fontData->_lineSpacing);
		}
	}
	_finishTextRendering();
}

// ---------------------------------------------------------------------------
// AppKit

void *CFontDrawable::_createGDIFont()
{
	NSString *family = [NSString stringWithLString:_fontName];
	CGFloat size = (CGFloat)((float)_fontSize * _fontScale);
	NSFontTraitMask traits = (_fontFlags & kFontBold) ? NSBoldFontMask : 0;
	if (_fontFlags & kFontItalic)
		traits |= NSItalicFontMask;
	NSFont *font = [[NSFontManager sharedFontManager] fontWithFamily:family traits:traits weight:9 size:size];
	if (font == nil)
		font = (_fontFlags & kFontBold) ? [NSFont boldSystemFontOfSize:size] : [NSFont systemFontOfSize:size];
	return [font retain];
}

void CFontDrawable::_deleteGDIFont(void *font)
{
	[(id)font release];
}

void *CFontDrawable::_createOffscreenImage()
{
	NSBitmapImageRep *rep = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL
	                                                                pixelsWide:_textureWidth
	                                                                pixelsHigh:_textureHeight
	                                                             bitsPerSample:8
	                                                           samplesPerPixel:4
	                                                                  hasAlpha:YES
	                                                                  isPlanar:NO
	                                                            colorSpaceName:NSDeviceRGBColorSpace
	                                                               bytesPerRow:_textureWidth * 4
	                                                              bitsPerPixel:32];
	[rep setSize:NSMakeSize(_textureWidth / _userSpaceScale, _textureHeight / _userSpaceScale)];
	memset([rep bitmapData], 0, _textureWidth * _textureHeight * 4);
	return rep;
}

void CFontDrawable::_deleteOffscreenImage(void *image)
{
	[(id)image release];
}

const void *CFontDrawable::_lockCharsImage()
{
	return [(NSBitmapImageRep *)_bitmap bitmapData];
}

void CFontDrawable::_unlockCharsImage() {}

// Lays the characters out in rows, measuring only or drawing them and
// recording where each one is.
HRESULT CFontDrawable::_paintAlphabet(void *image, void *font, unsigned char measureOnly)
{
	NSDictionary *attributes =
	    [NSDictionary dictionaryWithObjectsAndKeys:(id)font, NSFontAttributeName, [NSColor whiteColor],
	                                               NSForegroundColorAttributeName, nil];
	NSGraphicsContext *saved = [NSGraphicsContext currentContext];
	[NSGraphicsContext saveGraphicsState];
	[NSGraphicsContext
	    setCurrentContext:[NSGraphicsContext graphicsContextWithBitmapImageRep:(NSBitmapImageRep *)image]];
	float margin = (float)[@"X" sizeWithAttributes:attributes].height * 0.11f;
	float y = margin + margin;
	_charSpacing = 0;
	float width = (float)_textureWidth / _userSpaceScale;
	float height = (float)_textureHeight / _userSpaceScale;
	float lineHeight = 0;
	float x = margin;
	HRESULT result = 0;
	for (unsigned int c = _firstChar; c <= _lastChar; c++) {
		float lineStart = c == 0x20 ? 0 : margin;
		unsigned char byte = (unsigned char)c;
		NSString *s = [[NSString alloc] initWithBytes:&byte length:1 encoding:NSISOLatin1StringEncoding];
		NSSize size = [s sizeWithAttributes:attributes];
		if (c < 0x80 && lineHeight < size.height)
			lineHeight = (float)size.height;
		float right = (float)size.width + x + _charSpacing;
		if (right < 0)
			right = 0;
		if (width < (float)(unsigned int)right) {
			y = lineHeight + 2 + y;
			x = lineStart;
		}
		if (height < (float)size.height + y) {
			[s release];
			result = FH_E_ALPHABET_TOO_BIG;
			break;
		}
		if (!measureOnly) {
			float drawY = (((height - y) - (float)_fontSize) - margin) - 1;
			[s drawAtPoint:NSMakePoint(x, drawY) withAttributes:attributes];
			float *r = _charCoords + (c - _firstChar) * 4;
			r[0] = ((floorf(x) - 0.2f) - _charSpacing) / width;
			r[1] = (y + 0) / height;
			r[2] = (floorf((float)size.width) + x + 0.2f) / width;
			r[3] = ((float)size.height + y) / height;
			_scaleX = width / _fontScale;
			_scaleY = height / _fontScale;
		}
		[s release];
		x = floorf((float)size.width) + 3 + x;
	}
	[[NSGraphicsContext currentContext] flushGraphics];
	[NSGraphicsContext restoreGraphicsState];
	[NSGraphicsContext setCurrentContext:saved];
	return result;
}

// ---------------------------------------------------------------------------
// CFontDrawableData

CFontDrawableData::CFontDrawableData() : _flags(0), _lineSpacing(0), _strings(0), _color(0xffffffff) {}

void CFontDrawableData::dealloc()
{
	delete _strings;
	_strings = 0;
	CDrawableModelData::dealloc();
}

// Stages tagged 1 draw plain text, 2 shadowed text.
bool CFontDrawableData::_isStageOn(CShaderStage *stage, unsigned char *on)
{
	unsigned int flags;
	if (stage->_objectID == 1)
		flags = _flags ^ kTextShadowed;
	else if (stage->_objectID == 2)
		flags = _flags;
	else
		return false;
	*on = (flags >> 7) & 1;
	return true;
}

void CFontDrawableData::addCaptionInRect(LString *text, LRect rect, const Color *color)
{
	if (_strings == 0)
		_strings = new TArray<SStringEntry *, _TPtrArrayAddRemoveFunc<SStringEntry> >;
	SStringEntry *entry = new SStringEntry(text);
	entry->_type = 1;
	entry->_rect = rect;
	entry->_color = color ? (unsigned int)*color : _color;
	_strings->addObject(entry);
}

void CFontDrawableData::addCaptionAtPosition(LString *text, LPoint position, const Color *color)
{
	if (_strings == 0)
		_strings = new TArray<SStringEntry *, _TPtrArrayAddRemoveFunc<SStringEntry> >;
	SStringEntry *entry = new SStringEntry(text);
	entry->_type = 0;
	entry->_rect.origin = position;
	entry->_rect.size = LZeroSize;
	entry->_color = color ? (unsigned int)*color : _color;
	_strings->addObject(entry);
}

void CFontDrawableData::removeAllStrings()
{
	delete _strings;
	_strings = 0;
}
