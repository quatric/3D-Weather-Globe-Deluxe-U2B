// Screen-space drawables: sprites, glows, lens flares, billboards and text.

#ifndef FHSPRITES_H
#define FHSPRITES_H

#include "FHScene.h"

class CSpriteModelData;

// Writes the indices of `quads` quads of 4 vertices, two triangles each.
void FHFillQuadIndices(CIndexBuffer *ib, unsigned int quads);

// ---------------------------------------------------------------------------
// Sprites: quads around a point, drawn in projected space.

class CSpriteModel : public CBuffersOwner,
                     public IBuffersOwnerDataSource,
                     public IVertexBufferDataSource,
                     public IIndexBufferDataSource {
	LF_DECLARE_CLASS(CSpriteModel)
public:
	CSpriteModel();
	virtual void dealloc();
	virtual LClass *modelDataClass();
	virtual bool evalVisibilityParams(CDrawableModelData *data, Vector3 *center, float *distance);
	virtual long beginDrawing(CDrawableModelData *data);
	virtual CSpriteModel *getOwnerLObject() { return this; }
	virtual D3DPRIMITIVETYPE primitiveTypeForBufOwner(CBuffersOwner *) { return D3DPT_TRIANGLELIST; }
	virtual CVertexBuffer *vertexBufferForBufOwner(int stream, CBuffersOwner *owner);
	virtual CIndexBuffer *indexBufferForBufOwner(CBuffersOwner *owner);
	virtual unsigned int vertCountForBuffer(CVertexBuffer *) { return _spritesCount * 4; }
	virtual CShader *renderShaderForBuffer(CVertexBuffer *) { return renderProfile()->_shader; }
	virtual unsigned int streamNumberForBuffer(CVertexBuffer *) { return 0; }
	virtual void fillBuffer(CVertexBuffer *) {}
	virtual unsigned int indsCountForBuffer(CIndexBuffer *) { return _spritesCount * 6; }
	virtual CShader *renderShaderForBuffer(CIndexBuffer *) { return renderProfile()->_shader; }
	virtual bool use32BitsForBuffer(CIndexBuffer *) { return _spritesCount * 4 < 0x10000; }
	virtual void fillBuffer(CIndexBuffer *ib);
	// Writes the quads of a sprite centered on `center`.
	virtual void fillVertexBuffer(void *buffer, unsigned int stride, const Vector3 *center, const Vector2 *halfSize,
	                              CSpriteModelData *data);

	CSpriteModel *initWithParams(CRenderProfile *profile);

	unsigned int _spritesCount;
	CVertexBuffer *_vb;
	CIndexBuffer *_ib;
};

class CSpriteModelData : public CDrawableModelData {
	LF_DECLARE_CLASS(CSpriteModelData)
public:
	CSpriteModelData();
	virtual void storeSceneStateInStream(LData *stream, unsigned int flags);
	virtual const void *restoreSceneStateFromData(const void *data, unsigned int flags);
	void setPosition(const Vector3 *position) { _position = *position; }
	void setSpriteSize(const Vector2 *size) { _size = *size; }

	Vector3 _position;
	Vector2 _size;
};

class CGlowModelData : public CSpriteModelData {
	LF_DECLARE_CLASS(CGlowModelData)
public:
	CGlowModelData();
	virtual void storeSceneStateInStream(LData *stream, unsigned int flags);
	virtual const void *restoreSceneStateFromData(const void *data, unsigned int flags);
	virtual void setGlowBrightness(float brightness) { _brightness = brightness; }
	float currentGlowBrightness() const { return _brightness; }
	void setGlowColor(const Color *color) { _color = *color; }

	Color _color;
	float _brightness;
};

// Nine flares along the line through the screen center.
class CLensFlareModel : public CSpriteModel {
	LF_DECLARE_CLASS(CLensFlareModel)
public:
	CLensFlareModel();
	virtual LClass *modelDataClass();
	virtual bool evalVisibilityParams(CDrawableModelData *data, Vector3 *center, float *distance);
	virtual CLensFlareModel *getOwnerLObject() { return this; }
	virtual void fillVertexBuffer(void *buffer, unsigned int stride, const Vector3 *center, const Vector2 *halfSize,
	                              CSpriteModelData *data);
};

// ---------------------------------------------------------------------------
// Billboards: screen rectangles batched 512 to a draw.

struct CBillboardSingleData {
	CBillboardSingleData();
	void setScreenRect(LRect rect)
	{
		_rect = rect;
		_inViewport = false;
	}
	// In -1..1 viewport units.
	void setViewportRect(LRect rect)
	{
		_rect = rect;
		_inViewport = true;
	}
	void setRotationAngle(double angle) { _rotation = angle; }
	void setTrapetionFactor(float factor) { _trapetion = factor < -1 ? -1 : factor > 1 ? 1 : factor; }
	void setFlipHorizontally(unsigned char flip) { _flip = flip; }
	void setDiffuseColor(const Color *color)
	{
		if (color)
			_color = *color;
	}
	void setTextureScale(const Vector2 *scale)
	{
		if (scale)
			_texScale = *scale;
	}
	void setTextureOffset(const Vector2 *offset)
	{
		if (offset)
			_texOffset = *offset;
	}

	LRect _rect;
	bool _inViewport;
	unsigned int _color;
	double _rotation;
	float _trapetion;
	bool _flip;
	Vector2 _texScale;
	Vector2 _texOffset;
};

class CBillboardModelData : public ModelDataWithMatrix {
	LF_DECLARE_CLASS(CBillboardModelData)
public:
	CBillboardModelData() : _wrapTexture(false), _billboards(0) {}
	virtual void dealloc();
	void setWrapTexture(unsigned char wrap) { _wrapTexture = wrap; }
	void removeAllBillboards();
	void addBillboard(const CBillboardSingleData *billboard);
	// With `create`, adds billboards up to the index.
	CBillboardSingleData *billboardAtIndex(int index, unsigned char create);
	int billboardsCount() const { return _billboards ? _billboards->count() : 0; }

	bool _wrapTexture;
	TArray<CBillboardSingleData> *_billboards;
};

class CBillboardModel : public CBuffersOwner,
                        public IBuffersOwnerDataSource,
                        public IVertexBufferDataSource,
                        public IIndexBufferDataSource {
	LF_DECLARE_CLASS(CBillboardModel)
public:
	CBillboardModel();
	virtual void dealloc();
	virtual LClass *modelDataClass();
	virtual bool evalVisibilityParams(CDrawableModelData *data, Vector3 *center, float *distance);
	virtual void drawSubset(CDrawableModelData *data, long subset, long index);
	virtual CBillboardModel *getOwnerLObject() { return this; }
	virtual CVertexBuffer *vertexBufferForBufOwner(int stream, CBuffersOwner *owner);
	virtual D3DPRIMITIVETYPE primitiveTypeForBufOwner(CBuffersOwner *) { return D3DPT_TRIANGLELIST; }
	virtual CIndexBuffer *indexBufferForBufOwner(CBuffersOwner *owner);
	virtual unsigned int vertCountForBuffer(CVertexBuffer *) { return 0x800; }
	virtual CShader *renderShaderForBuffer(CVertexBuffer *);
	virtual unsigned int streamNumberForBuffer(CVertexBuffer *) { return 0; }
	virtual void fillBuffer(CVertexBuffer *) {}
	virtual unsigned int indsCountForBuffer(CIndexBuffer *) { return 0xc00; }
	virtual CShader *renderShaderForBuffer(CIndexBuffer *);
	virtual bool use32BitsForBuffer(CIndexBuffer *) { return vertCountForBuffer(_vb) > 0xffff; }
	virtual void fillBuffer(CIndexBuffer *ib);

	CBillboardModel *initWithRenderProfile(CRenderProfile *profile);
	void _startBillboardsRendering();
	void _renderSingleBB(const CBillboardSingleData &billboard);
	void _finishBillboardsRendering();

	CVertexBuffer *_vb;
	CIndexBuffer *_ib;
	LSize _frameSize;
	bool _rendering;
	int _count;	// quads written to the locked buffer, -1 when not locked
	char *_locked;
	unsigned int _vertexSize;
	int _positionOffset, _texcoordOffset, _colorOffset;
};

// ---------------------------------------------------------------------------
// Text: the characters 0x20-0xff painted into a texture with AppKit and
// drawn as quads.

// Text flags.
enum {
	kTextFullWidth = 0x1,	// the render target's width, centered
	kTextFullHeight = 0x2,	// the render target's height, centered
	kTextCenterH = 0x4,
	kTextCenterV = 0x8,
	kTextRight = 0x10,
	kTextEllipsize = 0x40,	// long lines end in "..."
	kTextShadowed = 0x80,	// selects the shader stage tagged 2 instead of 1
};

// Font flags.
enum { kFontBold = 1, kFontItalic = 2 };

// Walks the characters of a string a line at a time.
struct _textIterator {
	_textIterator(const LString *string);
	~_textIterator() { LRelease(_string); }
	bool hasMoreLines() const { return _p && *_p; }
	// The next character of the line, false at its end.
	bool nextChar(UniChar *c);
	int currentOffset() const;

	LString *_string;
	const UniChar *_p;
	bool _newLine;
};

class CFontDrawable : public CBuffersOwner,
                      public IBuffersOwnerDataSource,
                      public IVertexBufferDataSource,
                      public IIndexBufferDataSource,
                      public C2DTextureDataSource {
	LF_DECLARE_CLASS(CFontDrawable)
public:
	CFontDrawable();
	virtual void dealloc();
	virtual void instantiateSelf();
	virtual LClass *modelDataClass();
	virtual bool evalVisibilityParams(CDrawableModelData *, Vector3 *, float *) { return true; }
	virtual void drawSubset(CDrawableModelData *data, long subset, long index);
	virtual CFontDrawable *getOwnerLObject() { return this; }
	virtual D3DPRIMITIVETYPE primitiveTypeForBufOwner(CBuffersOwner *) { return D3DPT_TRIANGLELIST; }
	virtual CVertexBuffer *vertexBufferForBufOwner(int stream, CBuffersOwner *owner);
	virtual CIndexBuffer *indexBufferForBufOwner(CBuffersOwner *owner);
	virtual unsigned int vertCountForBuffer(CVertexBuffer *) { return 0x1000; }
	virtual CShader *renderShaderForBuffer(CVertexBuffer *) { return _profile->_shader; }
	virtual unsigned int streamNumberForBuffer(CVertexBuffer *) { return 0; }
	virtual void fillBuffer(CVertexBuffer *) {}
	virtual unsigned int indsCountForBuffer(CIndexBuffer *) { return 0x1800; }
	virtual CShader *renderShaderForBuffer(CIndexBuffer *) { return _profile->_shader; }
	virtual bool use32BitsForBuffer(CIndexBuffer *) { return false; }
	virtual void fillBuffer(CIndexBuffer *ib);
	virtual unsigned int widthOfTexture(C2DTexture *, CContext *);
	virtual unsigned int heightOfTexture(C2DTexture *, CContext *);
	virtual bool isDynamicTexture(CBaseTexture *, CContext *) { return false; }
	virtual D3DFORMAT textureFormat(CBaseTexture *, unsigned int, CContext *) { return D3DFMT_A8R8G8B8; }
	virtual void textureWasInstantiated(CBaseTexture *texture);
	virtual void _createCharsBitmap();
	virtual bool _charsBitmapIsCreated() { return _bitmap != 0; }
	virtual void _copyCharsBitmapToTexture();
	virtual void _deleteCharsBitmap();
	virtual void _startTextRendering();
	virtual void _finishTextRendering();
	virtual void _renderText(LString *text, const LRect &rect, const LSize &scale, unsigned int color,
	                         unsigned int flags, float lineSpacing);

	CFontDrawable *initWithHolderAndParameters(CHolder *holder, LString *fontName, unsigned int size,
	                                           unsigned int flags);
	LSize calcTextSize(const LString *text);
	float _charWidth(UniChar c) const
	{
		const float *r = _charCoords + (c - _firstChar) * 4;
		return (r[2] - r[0]) * _scaleX + _charSpacing * -2.0f;
	}
	float _charsHeight() const { return (_charCoords[3] - _charCoords[1]) * _scaleY; }
	bool _hasChar(unsigned int c) const { return _firstChar <= c && c <= _lastChar; }
	void _renderChar(UniChar c, const LRect *rect);
	bool _evalLineOffsets(LString *text, const LRect &rect, unsigned int flags, float lineSpacing,
	                      TArray<Vector2> *offsets);

	// AppKit (CFontDrawable.mm).
	void *_createGDIFont();
	void _deleteGDIFont(void *font);
	void *_createOffscreenImage();
	void _deleteOffscreenImage(void *image);
	const void *_lockCharsImage();
	void _unlockCharsImage();
	HRESULT _paintAlphabet(void *image, void *font, unsigned char measureOnly);

	unsigned int _fontFlags;
	LString *_fontName;
	unsigned int _fontSize;
	float _ellipsisWidth;
	CVertexBuffer *_vb;
	int _positionOffset, _colorOffset, _texcoordOffset;
	unsigned int _vertexSize;
	int _count;	// quads written to the locked buffer, -1 when not locked
	bool _rendering;
	char *_locked;
	unsigned int _color;
	C2DTexture *_texture;
	unsigned int _firstChar, _lastChar;
	float *_charCoords;	// left, top, right, bottom in the texture per character
	float _scaleX, _scaleY;	// texture units to pixels
	float _charSpacing;
	float _fontScale;	// the font is shrunk to fit the largest texture
	unsigned int _textureWidth, _textureHeight;
	void *_bitmap;	// NSBitmapImageRep
	void *_lockedImage;
	float _userSpaceScale;
	bool _oldSystem;
};

class CFontDrawableData : public CDrawableModelData {
	LF_DECLARE_CLASS(CFontDrawableData)
public:
	struct SStringEntry {
		SStringEntry(LString *string) : _string(LRetain(string)) {}
		~SStringEntry() { LRelease(_string); }
		LString *_string;
		unsigned int _color;
		int _type;	// 0 at a position, 1 in a rect
		LRect _rect;
	};

	CFontDrawableData();
	virtual void dealloc();
	virtual void storeSceneStateInStream(LData *stream, unsigned int flags)
	{
		CDrawableModelData::storeSceneStateInStream(stream, flags);
	}
	virtual const void *restoreSceneStateFromData(const void *data, unsigned int flags)
	{
		return CDrawableModelData::restoreSceneStateFromData(data, flags);
	}
	virtual bool _isStageOn(CShaderStage *stage, unsigned char *on);

	void setDrawFlags(unsigned int flags) { _flags = flags; }
	void setColor(Color color) { _color = color; }
	void setLineSpacing(float spacing) { _lineSpacing = spacing; }
	LSize calcTextSize(const LString *text) { return ((CFontDrawable *)_model)->calcTextSize(text); }
	void addCaptionInRect(LString *text, LRect rect, const Color *color);
	void addCaptionAtPosition(LString *text, LPoint position, const Color *color);
	void removeAllStrings();

	unsigned int _flags;
	float _lineSpacing;
	TArray<SStringEntry *, _TPtrArrayAddRemoveFunc<SStringEntry> > *_strings;
	unsigned int _color;
};

#endif
