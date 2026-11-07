// FishHook's rendering context: the objects that live in a GL context
// (CContextObject), the holder that tracks them for a view (CHolder), the
// context itself (CContext) with its draw cache, render targets, cameras,
// state blocks and render environment, and the C++ side of the 3D view.
#ifndef FHCONTEXT_H
#define FHCONTEXT_H

#include "FHDeviceEmu.h"
#include "LNotification.h"
#include "CThreadData.h"
#include "LSet.h"

class CHolder;
class CContext;
class CContextObject;
class CRenderTarget;
class CContextCamera;
class CRenderProfile;
class CRenderEnvironment;
class CClipPlane;
class CContextLight;
class CEnvironmentMap;
class CScene;
class CBaseTexture;
class CDrawCache;
class CStateBlock;
class BackBufferGrabber;
class LargeBBEmulator;
class C3DView;
class StPostponeProfilesInvalidation;
typedef void CDeviceFormat;	// never used: there is one device format

// D3D-style error codes the device-lost handling checks.
#define D3DERR_DEVICELOST ((HRESULT)0x88760868)
#define D3DERR_DEVICENOTRESET ((HRESULT)0x88760869)

// Notifications (their object is the CHolder, CRenderTarget or C3DView).
extern LConstantString ContextWillBeDetachedNotification;
extern LConstantString ContextWasAttachedNotification;
extern LConstantString targetWasAttached;
extern LConstantString targetWasDetached;
extern LConstantString FrameWillBeRenderedIn3DView;

// The FishHook framework bundle and its finalisation.
CFBundleRef getFishHookModule();
void setFishHookModule(CFBundleRef bundle);
void finalizeFishHook();

// The number of indices `primCount` primitives of `type` use.
unsigned int calcIndicesCount(D3DPRIMITIVETYPE type, unsigned int primCount);

struct SViewPort {
	int x, y, width, height;
	float minZ, maxZ;
};

// ---------------------------------------------------------------------------
// FHObject: an object created from a map whose "Class" entry names its class.
// The other entries are handed to processInitializationPair one by one.

extern LConstantString kFHObjectClassKey;
extern LConstantString kObjectIDKey;	// "ID": an id some scene objects and shader stages carry
// The keys of scene object descriptions.
extern LConstantString kObjectMatrixKey;
extern LConstantString kObjectNameKey;
extern LConstantString kObjectChildrenKey;
extern LConstantString kObjectAnimationKeysKey;
extern LConstantString kObjectMeshIDKey;
extern LConstantString kAnimationKeyTimeKey;
extern LConstantString kAnimationKeyTransformKey;

class FHObject : public LObject {
	LF_DECLARE_ABSTRACT_CLASS(FHObject)
public:
	virtual FHObject *initWithMapAndContext(const LMap *map, void *context) = 0;
	virtual void processInitializationPair(const LObject *value, const LComparable *key, const LMap *map) = 0;

	static FHObject *objectWithMap(const LMap *map, void *context);
};

// ---------------------------------------------------------------------------
// CContextObject: a resource with a GL part. The holder instantiates objects
// in priority order when a context arrives and releases them in reverse when
// it goes. Objects that manage themselves also drop and rebuild their GL
// part around a context reset.

typedef int TCtxObjPriority;

class CContextObject : public FHObject {
	LF_DECLARE_ABSTRACT_CLASS(CContextObject)
public:
	CContextObject(TCtxObjPriority priority = 0);
	virtual void dealloc();
	virtual CContextObject *initWithMapAndContext(const LMap *map, void *context);
	virtual void processInitializationPair(const LObject *value, const LComparable *key, const LMap *map);
	virtual void instantiateInContext(CContext *context);
	virtual void releaseContextDependedPart();
	virtual void purgeResources();
	virtual void restoreResources();
	virtual void instantiateSelf() = 0;
	virtual void releaseSelf() = 0;
	virtual void purgeSelf();
	virtual void restoreSelf();

	CContextObject *initWithHolder(CHolder *holder);
	void _processInitializationContext(void *context);
	CContext *context() const { return _context; }
	CHolder *holder() const { return _holder; }
	TCtxObjPriority priority() const { return _priority; }

	// Creates an object from a map; the holder is the creation context.
	static CContextObject *contextObjectWithMap(LMap *map, CHolder *holder);
	// The same from a file holding an archived map.
	static CContextObject *objectWithDescriptionFile(const LString *path, CHolder *holder);

	CContext *_context;
	bool _managesSelf;	// purge/restore drop and rebuild the GL part
	bool _restored;
	TCtxObjPriority _priority;
	CHolder *_holder;
	const char *_typeName;
};

struct CContextObjectComparator {
	// Orders by priority, then by address.
	static int compare(const void *a, const void *b, const void *context);
};

// ---------------------------------------------------------------------------
// CHolder: the context objects, textures, render profiles and named objects
// of one view, and the view's current context.

// Called on objects stored with setObjectForKey when the holder goes.
class CHolderObjectDelegate {
public:
	virtual void holderWillDie(CHolder *holder) = 0;
	virtual ~CHolderObjectDelegate() {}
};

struct _txtMapCallbacks {
	static unsigned int hash(const LString *k) { return k->hashValue(); }
	static bool equal(const LString *stored, const LString *k) { return stored->isEqualTo(k); }
	template <typename E> static void add(E *e) { e->key->retain(); }
	template <typename E> static void remove(E *e) { LRelease(e->key); }
	static const bool replace = true;
};
// Keys retained, textures not.
typedef TMap<LString *, CBaseTexture *, _txtMapCallbacks, _txtMapCallbacks> _holderTexturesMap;

class CHolder : public LObject {
	LF_DECLARE_CLASS(CHolder)
public:
	CHolder();
	virtual void dealloc();
	CHolder *initWithView(C3DView *view);

	C3DView *view() const { return _view; }
	CContext *context() const { return _context; }
	void *systemViewPtr();

	// Context objects created inside a block are instantiated together, in
	// priority order, when it ends.
	void beginAccumulationBlock();
	void endAccumulationBlock();
	LArray *currentAccumulationArray();
	void addContextObject(CContextObject *object);
	void removeContextObject(CContextObject *object);
	void adoptToContext(CContext *context);
	void resetCurrentContext(CDeviceFormat *format);

	CRenderProfile *renderProfileByFileDesc(LString *path);
	CRenderProfile *renderProfileByKey(LString *key);
	void removeRenderProfile(CRenderProfile *profile);
	CBaseTexture *textureByFileDesc(LString *path);
	void setTextureForFileDescr(CBaseTexture *texture, LString *path);
	void removeTextureWithFileDescr(LString *path);
	LObject *objectForKey(const LString *key);
	void setObjectForKey(LObject *object, const LString *key);
	void removeObjectForKey(const LString *key);

	void doAcceleratedRendering(bool postNotification, bool endFrame, bool present);
	LSize backBufferSize();
	LSize systemFrameSize();
	LSize virtualFrameSize();
	LSize fullFrameSize();
	LSize relativeFrameSize();
	void refreshView();
	void refreshViewFromScene(CScene *scene);
	void onBackbufferReady();
	bool setCurrentGLContext();
	void flushBuffer();
	void setBackBufferEmulator(LargeBBEmulator *emulator);
	LargeBBEmulator *backBufferEmulator() const { return _bbEmulator; }

	C3DView *_view;
	CContext *_context;
	int _accumulationDepth;
	CSharableResource _objectsLock;
	CHashSet *_objects;	// the context objects
	CSharableResource _texturesLock;
	_holderTexturesMap *_textures;
	CSharableResource _profilesLock;
	LMap *_profiles;
	CSharableResource _namedObjectsLock;
	LMap *_namedObjects;
	CSharableResource _unusedLock;
	CLocalThreadData _threadData;
	LargeBBEmulator *_bbEmulator;
};

// ---------------------------------------------------------------------------
// CContext: one GL context and its device, with the world transform and the
// matrices derived from it and the current render target.

class CContext {
public:
	CContext(CHolder *holder, CDeviceFormat *format);
	virtual ~CContext();
	virtual void setDefaultDrawParameters();
	virtual void setZBufferEnable(bool enable);
	virtual void setSoftwareVertexProcessing(bool software);

	DeviceEmu *device() const { return _device; }
	CContextCaps *caps() const { return _caps; }
	CHolder *holder() const { return _holder; }
	CDrawCache *drawCache() const { return _drawCache; }
	CRenderEnvironment *renderEnvironment() const { return _renderEnvironment; }
	CRenderTarget *currentRenderTarget() const { return _currentRenderTarget; }
	CRenderTarget *contextRenderTarget() const { return _contextRenderTarget; }

	bool canBeResetToFormat(CDeviceFormat *) { return true; }
	bool checkDeviceFormat(D3DFORMAT format, unsigned int usage, D3DRESOURCETYPE type);
	bool checkDepthStencilMatch(D3DFORMAT) { return false; }
	D3DFORMAT FindZBufferFormat(long minDepth, long maxDepth, long minStencil, long maxStencil);
	D3DFORMAT FindRenderBuffFormat(unsigned int usage, D3DRESOURCETYPE type, long minAlpha, long minColor,
	                               bool noAlpha);
	void FindShadowMapsFormat();
	HRESULT testCooperativeLevel() { return DeviceEmu::TestCooperativeLevel(); }
	bool resetContext(CDeviceFormat *format);
	void resetRenderProfiles();

	HRESULT beginFrame();
	HRESULT endFrame();
	HRESULT presentFrame();
	void flushBuffer();
	void setRenderTarget(CRenderTarget *target);
	void clearCurrentRenderTarget();
	CContextCamera *mainCamera() const;
	void setMainCamera(CContextCamera *camera);

	void setWorldTransform(const Matrix *m);
	void passWorldMatrixToDevIfNeeded();
	void invalViewProjDepedences() { _validMatrices &= ~0x3f8u; }
	void _setTransforms(CRenderTarget *target);
	const Matrix *worldMatrix() const { return &_world; }
	const Matrix *worldMatrixTransp();
	const Matrix *invWorldMatrix();
	const Matrix *invWorldMatrixTransp();
	const Matrix *worldViewMatrixTransp();
	const Matrix *viewMatrixTransp();
	const Matrix *viewProjMatrix();
	const Matrix *worldViewProjMatrixTransp();
	const Matrix *viewProjMatrixTransp();
	const Matrix *invViewProjMatrixTransp();

	int boxIntersectionTest(const Vector3 *min, const Vector3 *max, bool useContextTarget);
	int boxIntersectionTest(const Vector3 *min, const Vector3 *max, const Matrix *m, bool useContextTarget);
	void projectVector(Vector3 *out, const Vector3 *v, const Matrix *m, float *invW);
	bool spriteProbe(const Vector3 *center, float width, float height, const Matrix *m, Vector3 *projected,
	                 float *invW);
	bool sphereProbe(const Vector3 *center, float radius, const Matrix *m, Vector3 *projected, float *invW);

	// 512x2 lookup textures for the specular and Fresnel terms.
	Texture2DEmu *specularTable();
	Texture2DEmu *fresnelTable();

	DeviceEmu *_device;
	CContextCaps *_caps;
	bool _lost;
	int _shadowMapType;	// 0 none, 1 depth texture, 2 float texture
	D3DFORMAT _shadowMapFormat;
	unsigned int _drawCallsCount;
	unsigned int _primitivesCount;
	CRenderEnvironment *_renderEnvironment;
	CStateBlock *_defaultStateBlock;
	CDrawCache *_drawCache;
	CRenderTarget *_currentRenderTarget;
	CRenderTarget *_contextRenderTarget;
	bool _worldChanged;
	Matrix _world;
	Matrix _worldTransp, _invWorld, _invWorldTransp, _worldViewTransp, _worldViewProj, _worldViewProjTransp;
	Matrix _viewTransp, _viewProj, _viewProjTransp, _invViewProjTransp;
	unsigned int _validMatrices;
	unsigned char _softwareVP;
	Texture2DEmu *_specularTable;
	Texture2DEmu *_fresnelTable;
	CHolder *_holder;
};

// ---------------------------------------------------------------------------
// CDrawCache: the device calls of a frame. Draw calls are passed straight on.

class CDrawCache : public LObject {
	LF_DECLARE_CLASS(CDrawCache)
public:
	CDrawCache();
	virtual void dealloc();
	CDrawCache *initWithContext(CContext *context);

	static void beginFrame() {}
	static void endFrame() {}
	void flush() {}
	HRESULT beginDrawing() { return D3D_OK; }
	HRESULT endDrawing() { return D3D_OK; }
	HRESULT present();
	void clear(const Color &color, unsigned int flags);
	void setViewport(const SViewPort *viewport);

	void startTexturesSetup();
	void endTexturesSetup();
	void applyStateBlock(CStateBlock *block);
	void setTransform(D3DTRANSFORMSTATETYPE type, const _SMatrix *m) { _device->SetTransform(type, m); }
	void setRenderState(D3DRENDERSTATETYPE state, unsigned int value) { _device->SetRenderState(state, value); }
	void setTextureStageState(unsigned int stage, D3DTEXTURESTAGESTATETYPE type, unsigned int value)
	{
		_device->SetTextureStageState(stage, type, value);
	}
	void setSamplerState(unsigned int stage, D3DSAMPLERSTATETYPE type, unsigned int value)
	{
		_device->SetSamplerState(stage, type, value);
	}
	void setTexture(unsigned int stage, BaseTextureEmu *texture);
	void setClipPlane(unsigned int, const Plane *) {}
	void setVertexShader(DeviceShaderEmu *shader) { _device->SetVertexShader(shader); }
	void setPixelShader(DeviceShaderEmu *shader) { _device->SetPixelShader(shader); }
	void setVSConstant(unsigned int start, const float *data, unsigned int count)
	{
		_device->SetVertexShaderConstantF(start, data, count);
	}
	void setPSConstant(unsigned int start, const float *data, unsigned int count)
	{
		_device->SetPixelShaderConstantF(start, data, count);
	}
	void setStreamSource(long stream, VertexBufferEmu *vb, unsigned int stride)
	{
		_device->SetStreamSource(stream, vb, 0, stride);
	}
	void setVertexBuffer(VertexBufferEmu *vb, unsigned int stride) { _device->SetStreamSource(0, vb, 0, stride); }
	void setIndexBuffer(IndexBufferEmu *ib) { _device->SetIndices(ib); }
	void setVertexDeclaration(VertexDeclEmu *decl) { _device->SetVertexDeclaration(decl); }
	void setSoftwareVertexProcessing(bool) {}
	void drawPrimitive(D3DPRIMITIVETYPE type, unsigned int startVertex, unsigned int primCount);
	void drawIndexedPrimitive(D3DPRIMITIVETYPE type, int baseVertexIndex, unsigned int minIndex,
	                          unsigned int numVertices, unsigned int startIndex, unsigned int primCount);

	CContext *_context;
	DeviceEmu *_device;
	unsigned int _texturesCount;
	bool *_texturesSet;	// stages given a texture since startTexturesSetup
};

// ---------------------------------------------------------------------------
// Scene state: objects whose state a scene can record and play back.

class CSceneStatePropertyOwner {
public:
	CSceneStatePropertyOwner();
	virtual ~CSceneStatePropertyOwner();
	virtual void storeSceneStateInStream(LData *stream, unsigned int flags) = 0;
	// Returns the data after this object's state.
	virtual const void *restoreSceneStateFromData(const void *data, unsigned int flags) = 0;
	virtual void renderSelfHolderInScene(CScene *, unsigned int) {}

	void wasRegisteredInScene(CScene *scene);
	void hostSceneWillDie(CScene *scene);
	bool isRegisteredInScene(CScene *scene);

	static void storeFloatInStream(float f, LData *stream) { stream->appendBytes(&f, 4); }
	static void storeDoubleInStream(double d, LData *stream) { stream->appendBytes(&d, 8); }
	static void storeIntInStream(int i, LData *stream) { stream->appendBytes(&i, 4); }
	static void storeMatrixInStream(const Matrix *m, LData *stream) { stream->appendBytes(m, 0x40); }
	static void storeColorInStream(const Color *c, LData *stream) { stream->appendBytes(c, 0x10); }
	static void storeVectorInStream(const Vector2 *v, LData *stream) { stream->appendBytes(v, 8); }
	static void storeVectorInStream(const Vector3 *v, LData *stream) { stream->appendBytes(v, 0xc); }
	static void storeVectorInStream(const Vector4 *v, LData *stream) { stream->appendBytes(v, 0x10); }
	static void storeStringInStream(LString *s, LData *stream);
	static const void *loadFloatFromStream(const void *data, float *f) { return _load(data, f, 4); }
	static const void *loadDoubleFromStream(const void *data, double *d) { return _load(data, d, 8); }
	static const void *loadIntFromStream(const void *data, int *i) { return _load(data, i, 4); }
	static const void *loadMatrixFromStream(const void *data, Matrix *m) { return _load(data, m, 0x40); }
	static const void *loadColorFromStream(const void *data, Color *c) { return _load(data, c, 0x10); }
	static const void *loadVectorFromStream(const void *data, Vector2 *v) { return _load(data, v, 8); }
	static const void *loadVectorFromStream(const void *data, Vector3 *v) { return _load(data, v, 0xc); }
	static const void *loadVectorFromStream(const void *data, Vector4 *v) { return _load(data, v, 0x10); }
	static const void *loadStringFromStream(const void *data, LString **s);
	static const void *_load(const void *data, void *out, unsigned int size)
	{
		memcpy(out, data, size);
		return (const char *)data + size;
	}

	const char *_stateOwnerName;
	TArray<CScene *> *_scenes;
	bool _storesState;
};

// CScene::unregisterStateOwner, for owners that go before their scenes.
void FHSceneUnregisterStateOwner(CScene *scene, CSceneStatePropertyOwner *owner);

// ---------------------------------------------------------------------------
// Cameras.

class CContextCamera : public LObject, public CSceneStatePropertyOwner {
	LF_DECLARE_CLASS(CContextCamera)
public:
	CContextCamera();
	virtual CContextCamera *init();
	virtual void dealloc();
	virtual const Vector3 *position() const { return &_position; }
	virtual void setPosition(const Vector3 *v);
	virtual const Vector3 *viewDirection() const { return &_direction; }
	virtual void setViewDirection(const Vector3 *v);
	virtual const Vector3 *viewUpVector() const { return &_up; }
	virtual void setViewUpVector(const Vector3 *v);
	virtual void setClipDistances(float nearZ, float farZ);
	virtual void setFOV(float fov);
	virtual void projectionMatrixForAspect(float aspect, Matrix *m) const;
	virtual void recalcViewMatrix() const;
	virtual void invalProjMatrixDependences() const;
	virtual void storeSceneStateInStream(LData *stream, unsigned int flags);
	virtual const void *restoreSceneStateFromData(const void *data, unsigned int flags);
	virtual void renderSelfHolderInScene(CScene *scene, unsigned int flags);
	virtual void attachToRenderTarget(CRenderTarget *target);
	virtual void detachFromRenderTarget() { _target = 0; }

	const Matrix *viewMatrix() const;
	float nearClip() const { return _near; }
	float farClip() const { return _far; }
	float fov() const { return _fov; }
	CRenderTarget *renderTarget() const { return _target; }

	// The stored state, in this order.
	Vector3 _position;
	Vector3 _direction;
	Vector3 _up;
	float _near, _far, _fov;
	mutable Matrix _view;
	mutable bool _viewValid;
	CRenderTarget *_target;
};

class COrthoCamera : public CContextCamera {
	LF_DECLARE_CLASS(COrthoCamera)
public:
	COrthoCamera();
	virtual COrthoCamera *init();
	virtual void setFOV(float) {}
	virtual void projectionMatrixForAspect(float aspect, Matrix *m) const;
	virtual void storeSceneStateInStream(LData *stream, unsigned int flags);
	virtual const void *restoreSceneStateFromData(const void *data, unsigned int flags);
	void setFrustumSize(LSize size);

	Vector2 _frustumSize;
};

// ---------------------------------------------------------------------------
// Render targets: where a frame goes, with its camera and fog/clear colours.
// A frame can be rendered in segments (for pictures bigger than the view);
// the projection then covers one segment.

class CRenderTarget : public CContextObject {
	LF_DECLARE_CLASS(CRenderTarget)
public:
	CRenderTarget();
	virtual void dealloc();
	virtual void instantiateSelf();
	virtual void releaseSelf() { _context = 0; }
	virtual void setClearColor(Color color) { _clearColor = color; }
	virtual void setFogColor(Color color);
	virtual void setCamera(CContextCamera *camera);
	virtual BackBufferGrabber *detachBackBufferGrabber(LSize size, D3DTEXTUREFILTERTYPE filter);
	virtual void attachToContext();
	virtual void detachFromContext();
	virtual void recalcViewMatrix(Matrix *m) const;
	virtual void recalcProjectionMatrix(Matrix *m) const;
	virtual float viewportAspect() const { return (float)_width / (float)_height; }
	virtual bool findBufferFromats(D3DFORMAT &color, D3DFORMAT &depth);

	CRenderTarget *initWithRectParameters(long width, long height, bool useZBuffer, CHolder *holder);
	Color fogColor() const { return _fogColor; }
	Color clearColor() const { return _clearColor; }
	CContextCamera *camera() const { return _camera; }
	bool usesZBuffer() const { return _useZBuffer; }
	const Matrix *viewMatrix() const;
	const Matrix *projectionMatrix() const;
	const Matrix *viewProjMatrix() const;
	const SViewFrustum *viewFrustum() const;
	const SViewPort *viewport() const;
	void invalViewProjDepedences();
	void setOutSegmentsCount(unsigned int cols, unsigned int rows);
	void setOutSegment(unsigned int col, unsigned int row);
	void getOutSegmentsCount(int *cols, int *rows) const;
	void getOutSegment(int *col, int *row) const;
	void adjustProjectionMatrixToCurrentSegment(Matrix *out, const Matrix *m) const;

	long _width, _height;
	Color _fogColor;
	Color _clearColor;
	long _minColorBits, _maxColorBits;
	long _minDepthBits, _maxDepthBits, _minStencilBits, _maxStencilBits;
	D3DFORMAT _colorFormat;
	D3DFORMAT _depthFormat;
	bool _useZBuffer;
	CContextCamera *_camera;
	mutable SViewFrustum _frustum;
	mutable SViewPort _viewport;
	mutable Matrix _projection;
	mutable Matrix _view;
	mutable Matrix _viewProj;
	unsigned int _segCols, _segRows, _segCol, _segRow;
	mutable bool _viewportValid, _frustumValid, _viewValid, _projectionValid, _viewProjValid;
};

// The view's own frame buffer.
class CContextRenderTarget : public CRenderTarget {
	LF_DECLARE_CLASS(CContextRenderTarget)
public:
	CContextRenderTarget();
	virtual void dealloc();
	virtual void instantiateSelf() {}
	virtual void releaseSelf() {}
	virtual void purgeSelf() {}
	virtual void restoreSelf();
	virtual float viewportAspect() const;
	CContextRenderTarget *initWithContext(CContext *context);
};

// ---------------------------------------------------------------------------
// Grabbing the back buffer (pictures and movies).

class BackBufferGrabber : public CContextObject {
	LF_DECLARE_CLASS(BackBufferGrabber)
public:
	BackBufferGrabber();
	virtual void dealloc();
	virtual void instantiateSelf();
	virtual void releaseSelf();
	virtual bool grab();
	// Notification selectors; args holds the LNotification.
	virtual intptr_t _rtWasUpdated(char *args);
	virtual intptr_t _contextWasAttached(char *args);
	virtual intptr_t _contextWillBeDetached(char *args);

	BackBufferGrabber *initWithParams(CRenderTarget *target, LSize size, D3DTEXTUREFILTERTYPE filter);
	void setAutoGrabbing(bool autoGrab);
	LSize size() const { return LMakeSize((float)_width, (float)_height); }
	D3DFORMAT grabbedBufferFormat() const { return _format; }
	void lockGrabbedData(D3DLOCKED_RECT *rect);
	void unlockGrabbedData();

	bool _autoGrabbing;
	CRenderTarget *_target;
	unsigned int _width, _height;	// the grabbed picture's size
	D3DFORMAT _format;
	D3DTEXTUREFILTERTYPE _filter;
	CSharableResource _dataLock;
	StSharableLocker *_dataLocker;
	GLenum _textureTarget;
	GLuint _texture;
	float _readWidth, _readHeight;	// the size of the last read
	void *_readBuffer;
	void *_scaledBuffer;
	void *_scaleTempBuffer;
	bool _pboSized;
	GLuint _pbo;
};

class IBBEmulatorDelegate {
public:
	virtual void renderingWasStarted(LargeBBEmulator *) {}
	virtual void renderingWasFinished(LargeBBEmulator *) {}
	virtual void processBackBufferSegment(LargeBBEmulator *, const D3DLOCKED_RECT *, unsigned int, unsigned int) {}
	virtual ~IBBEmulatorDelegate() {}
};

// Renders a picture bigger than the view in segments of at most the
// maximum texture size, grabbing each.
class LargeBBEmulator : public LObject {
	LF_DECLARE_CLASS(LargeBBEmulator)
public:
	LargeBBEmulator();
	virtual void dealloc();
	virtual void onBackbufferReady();
	LargeBBEmulator *initWithHolder(CHolder *holder);

	IBBEmulatorDelegate *delegate() const { return _delegate; }
	void setDelegate(IBBEmulatorDelegate *delegate) { _delegate = delegate; }
	LSize size() const { return _size; }
	LSize virtualFrameSize() const { return _virtualFrameSize; }
	LSize backBufferSize() const { return _segmentSize; }
	unsigned int bitmapDataSize() const { return _bitmapDataSize; }
	unsigned int bufferRowSize() const { return _rowSize; }
	D3DFORMAT bufferFormat() const { return _format; }
	void setBufferParameters(LSize segmentSize, unsigned int cols, unsigned int rows, LSize virtualFrameSize);
	void setBufferParametersSimple(LSize size, LSize virtualFrameSize);
	void startGrabbing();
	void stopGrabbing();
	void grabSnapshotSubimage(void *buffer, int rowBytes, int col, int row);
	void grabSnapshot(void *buffer, int rowBytes);
	LData *createSingleSnapshot();

	CHolder *_holder;
	LSize _size;
	LSize _virtualFrameSize;
	LSize _segmentSize;
	unsigned int _cols, _rows;
	unsigned int _bitmapDataSize;
	unsigned int _rowSize;
	D3DFORMAT _format;
	IBBEmulatorDelegate *_delegate;
	bool _savedTimerUpdate;
	BackBufferGrabber *_grabber;
};

// Grabs a big picture one segment per tryRender call.
class StBBGrabbingAsync {
public:
	class IGrabCB {
	public:
		virtual void setOutSize(LSize size) = 0;
		virtual void dataIsReady(const void *bytes, unsigned int firstRow, unsigned int rowsCount, unsigned int rowBytes) = 0;
		virtual ~IGrabCB() {}
	};

	StBBGrabbingAsync(LargeBBEmulator *emulator, LSize size, LSize virtualFrameSize, IGrabCB *callback);
	~StBBGrabbingAsync();
	void reset() { _segment = 0; }
	float renderProgress() const;
	bool tryRender();

	LargeBBEmulator *_emulator;
	IGrabCB *_callback;
	LData *_rowsData;	// one row of segments
	unsigned int _rowBytes;
	unsigned int _segmentRowBytes;
	int _segment;
	int _cols, _rows;
	int _segmentWidth, _segmentHeight;
};

// ---------------------------------------------------------------------------
// CRenderEnvironment: the lights, clip planes and environment map of a frame
// and the render profiles they affect.

class CRenderEnvironment {
public:
	CRenderEnvironment(CContext *context);
	virtual ~CRenderEnvironment();

	void endFrame();
	void beginFrameWithUsedProfiles(LSet *profiles);
	void invalidateProfile(CRenderProfile *profile);
	void addLight(CContextLight *light, CRenderProfile *profile);
	void removeLight(CContextLight *light, CRenderProfile *profile);
	void removeAllLights();
	void addClipPlane(CClipPlane *plane, CRenderProfile *profile);
	void removeClipPlane(CClipPlane *plane, CRenderProfile *profile);
	void removeAllClipPlanes();
	void setCurrentEnvMap(CEnvironmentMap *map);
	CEnvironmentMap *currentEnvMap() const { return _envMap; }
	int activeLightsCount() const { return _lights->count(); }
	CContextLight *activeLightAtIndex(int index) const { return (CContextLight *)_lights->objectAtIndex(index); }
	const D3DLIGHT9 *environmentLightAtIndex(int index) const;
	int clipPlanesCount() const { return _clipPlanes->count(); }
	CClipPlane *clipPlaneAtIndex(int index) const { return (CClipPlane *)_clipPlanes->objectAtIndex(index); }
	class CShadowEnvMap *shadowMapAtIndex(long index);

	CContext *_context;
	LArray *_usedProfiles;
	CRenderProfile *_activeProfile;
	Color _ambient;
	LArray *_lights;
	LArray *_clipPlanes;
	int _clipPlanesCount;	// "ClippingIsActive"
	int _shadowMapUpdateMode;	// the "ShadowMapUpdateMode" shader variable
	int _shadowMapsCount;	// "ShadowMapsCount"
	CEnvironmentMap *_envMap;
	TArray<StPostponeProfilesInvalidation *> *_postponers;
};

// ---------------------------------------------------------------------------
// CStateBlock: device states recorded into a StateBlockEmu.

struct SStateItemDescr {
	int type;	// 1 render state, 2 texture stage state, 3 sampler state
	unsigned int a, b, c;	// state and value, or stage, state and value
};

class CStateBlock : public CContextObject {
	LF_DECLARE_CLASS(CStateBlock)
public:
	CStateBlock();
	virtual void dealloc();
	virtual void instantiateSelf();
	virtual void releaseSelf();
	virtual void applySelf();

	CStateBlock *initWithHolder(CHolder *holder);
	CStateBlock *initWithStateDescriptors(const SStateItemDescr *items, long count, CHolder *holder);
	CStateBlock *initWithArray(LArray *array, CHolder *holder);
	// Each element is an array of strings: the item type then its values.
	void setStaticStatesArray(LArray *array);
	bool beginStateRecording();
	void endStateRecording();
	void appendStates(const SStateItemDescr *items, long count, DeviceEmu *device);

	static CStateBlock *defaultStateWithContext(CContext *context);
	static bool renderStateRequiresFloatParam(int state);
	static bool samplerStateRequiresFloatParam(int state) { return state == 8; }
	static bool txtStageStateRequiresFloatParam(unsigned int state);
	static bool dxConstantByString(LString *name, long &value);
	static void createMapsIfNeeded();

	static const SStateItemDescr _sDefaultStateDescr[];
	static const long _sDefaultStateItemsCount;

	StPointerBlock *_itemsBlock;
	SStateItemDescr *_items;
	long _itemsCount;
	StateBlockEmu *_block;
};

// Records the device calls of a block into a state block.
class StStateRecordZone {
public:
	StStateRecordZone(CStateBlock *block) : _retained(LRetain(block)), _block(block)
	{
		if (_block)
			_block->beginStateRecording();
	}
	~StStateRecordZone()
	{
		if (_block)
			_block->endStateRecording();
		LRelease(_retained);
	}
	CStateBlock *_retained;
	CStateBlock *_block;
};

// ---------------------------------------------------------------------------
// C3DView: the C++ side of a 3D view (C3DViewWrapper, an NSOpenGLView).

class C3DView : public LObject {
	LF_DECLARE_CLASS(C3DView)
public:
	C3DView();
	virtual void dealloc();
	virtual void recreateContext(CDeviceFormat *format);
	virtual bool isDoubleBuffered();
	virtual void drawAccelerated(bool postNotification, bool endFrame, bool present);
	virtual void drawInPreparedEnvironment();
	virtual bool mouseDown(unsigned int modifiers, LPoint where);
	virtual bool mouseMove(unsigned int modifiers, LPoint where);
	virtual bool mouseUp(unsigned int modifiers, LPoint where);
	virtual bool onMouseLeave(unsigned int modifiers, LPoint where);
	virtual bool mouseDblClick(unsigned int modifiers, LPoint where);
	virtual bool mouseWheel(short delta, unsigned int modifiers, LPoint where);
	virtual bool mouseDragged(unsigned int modifiers, LPoint where);
	virtual bool dragStarted(unsigned int modifiers, LPoint where);
	virtual bool dragFinished(unsigned int modifiers, LPoint where);
	virtual bool canBecameInFocus();
	virtual bool becameInFocus();
	virtual bool resignFocus();
	virtual bool onKeyDown(unsigned int keycode, unsigned int modifiers);
	virtual bool onKeyUp(unsigned int keycode, unsigned int modifiers);

	C3DView *initWithParams(void *systemView, CDeviceFormat *format);
	void adjustToSystemView(void *systemView, CDeviceFormat *format);
	CHolder *holder() const { return _holder; }
	CContext *context() const { return _context; }
	CScene *scene() const { return _scene; }
	void setScene(CScene *scene);
	void *systemView() const { return _sysView; }
	LRect frame();
	void refresh();
	bool setCurrentGLContext();
	void flushBuffer();
	float userSpaceScaleFactor();
	bool _windowIsVisible();
	void updateFPS();
	float fps() const { return _fps; }
	void enterCaptureModeWithSize(LSize size);
	void exitCaptureMode();

	bool _deviceLost;
	void *_sysView;	// C3DViewWrapper *
	void *_contextView;	// the view the context was made for
	CContext *_context;
	CHolder *_holder;
	CScene *_scene;
	unsigned int _framesCount;
	double _lastFPSTime;
	float _fps;
	float _mpps;	// millions of primitives a second
	float _unused;
	LSize _savedSize;
	LPoint _savedOrigin;
	void *_savedSuperview;
	void *_cachedImageView;
	double _lastPrintTime;
};

// ---------------------------------------------------------------------------
// GLBugFixes: works around drivers that need a vertex program to have been
// used for texture coordinates to reach fragment programs.

class GLBugFixes : public CContextObject {
	LF_DECLARE_CLASS(GLBugFixes)
public:
	GLBugFixes();
	virtual void instantiateSelf();
	virtual void releaseSelf();
	void drawVertexProgramTexcoordFix();

	GLuint _program;
};

#endif
