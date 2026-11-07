// FHDeviceEmu
//
// FishHook's Direct3D 9 emulation on OpenGL: the renderer is written against
// a D3D-like device (DeviceEmu) and resources (textures, surfaces, vertex and
// index buffers, vertex declarations, ARB shaders, state blocks), which this
// layer maps onto the fixed-function and ARB-program OpenGL of macOS.
// Device state is recorded on the Set* calls and applied lazily when drawing.

#ifndef FHDEVICEEMU_H
#define FHDEVICEEMU_H

#define GL_GLEXT_PROTOTYPES 1
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include <vector>

#include "LObject.h"
#include "LData.h"
#include "LArray.h"
#include "LMap.h"
#include "LCleanupTask.h"
#include "FHD3DTypes.h"
#include "FHMath.h"

using namespace LF;

class CContext;
class CContextCaps;
class DeviceEmu;
class BaseTextureEmu;
class StateBlockEmu;

// ---------------------------------------------------------------------------
// COM-style reference counting on top of LObject's.

class IUnknownEmu : public LObject {
	LF_DECLARE_CLASS(IUnknownEmu)
public:
	unsigned int AddRef()
	{
		retain();
		return retainCount();
	}
	unsigned int Release()
	{
		unsigned int n = retainCount() - 1;
		release();
		return n;
	}
	virtual IUnknownEmu *getOwnerLObject() { return this; }
};

// Releases *p (an IUnknownEmu) and clears it.
template <typename T> static inline void FHSafeRelease(T *&p)
{
	if (p) {
		p->Release();
		p = 0;
	}
}
// Stores `value` in `slot` with an AddRef, releasing the old one.
template <typename T> static inline void FHAssign(T *&slot, T *value)
{
	if (value)
		value->AddRef();
	FHSafeRelease(slot);
	slot = value;
}

class ResourceEmu : public IUnknownEmu {
	LF_DECLARE_ABSTRACT_CLASS(ResourceEmu)
public:
	ResourceEmu() : _device(0), _pool(D3DPOOL_DEFAULT), _usage(0) {}
	ResourceEmu *initWithParams(unsigned int usage, D3DPOOL pool, DeviceEmu *device);
	virtual D3DRESOURCETYPE GetType() = 0;
	// Kept in client memory (software processing or the system-memory pool).
	bool isInSystemMem() const { return (_usage & D3DUSAGE_SOFTWAREPROCESSING) || _pool == D3DPOOL_SYSTEMMEM; }
	DeviceEmu *device() const { return _device; }

	DeviceEmu *_device;
	D3DPOOL _pool;
	unsigned int _usage;
};

// One mip level (or cube face) of a texture.
class SurfaceEmu : public ResourceEmu {
	LF_DECLARE_CLASS(SurfaceEmu)
public:
	SurfaceEmu();
	virtual void dealloc();
	virtual D3DRESOURCETYPE GetType() { return D3DRTYPE_SURFACE; }

	// `sysMem`, when given, is the level's client storage (kept by the texture).
	SurfaceEmu *initWithParams(void *sysMem, GLenum target, unsigned int width, unsigned int height,
	                           unsigned int level, D3DFORMAT format, BaseTextureEmu *texture,
	                           unsigned int usage, D3DPOOL pool, DeviceEmu *device);
	HRESULT LockRect(D3DLOCKED_RECT *locked, const RECT *rect, unsigned int flags);
	HRESULT UnlockRect();
	HRESULT GetDesc(D3DSURFACE_DESC *desc);

	unsigned int _width, _height;
	D3DFORMAT _format;
	D3DMULTISAMPLE_TYPE _multiSample;
	unsigned int _multiSampleQuality;
	unsigned int _level;
	GLenum _target;
	GLuint _pbo;	// pixel buffer used for locking, if PBOs are on
	void *_sysMem;	// client storage, or 0
	BaseTextureEmu *_texture;	// not retained; cleared by the texture
	unsigned int _pitch;
	void *_lockedBits;	// while locked
	unsigned int _lockFlags;
	unsigned int _rowLength;
	unsigned int _size;
};

class BaseTextureEmu : public ResourceEmu {
	LF_DECLARE_ABSTRACT_CLASS(BaseTextureEmu)
public:
	BaseTextureEmu();
	virtual void dealloc();
	// Makes the texture current on a texture unit and enables its target.
	virtual void bind(unsigned int stage);
	virtual BaseTextureEmu *initWithParams(GLenum target, unsigned int width, unsigned int height,
	                                       D3DFORMAT format, unsigned int levels, unsigned int usage,
	                                       D3DPOOL pool, DeviceEmu *device);
	static void unbindAll(unsigned int stage);

	static GLenum internalTextureFormatFromD3DFormat(D3DFORMAT format);
	static void sourceFormatFromD3DFormat(D3DFORMAT format, GLenum *srcFormat, GLenum *srcType);
	// Uploads a surface's pixels (after an unlock).
	void _setImageData(SurfaceEmu *surface);
	unsigned int GetLevelCount() const { return _levels; }
	GLenum target() const { return _target; }
	GLuint textureName() const { return _name; }

	GLuint _name;
	GLenum _target;
	D3DFORMAT _format;
	unsigned int _width, _height;
	unsigned int _levels;
	TLArray<SurfaceEmu *> *_surfaces;
	void *_sysMemBuffer;	// client storage for all levels, if kept
	unsigned int _sysMemSize;
	bool _useTextureRange;
	bool _keepSysMem;
	bool _usePBO;
	bool _canMipmap;
};

class Texture2DEmu : public BaseTextureEmu {
	LF_DECLARE_CLASS(Texture2DEmu)
public:
	virtual D3DRESOURCETYPE GetType() { return D3DRTYPE_TEXTURE; }
	static void unbindAll() { glDisable(GL_TEXTURE_2D); }

	HRESULT GetSurfaceLevel(unsigned int level, SurfaceEmu **surface);
	HRESULT LockRect(unsigned int level, D3DLOCKED_RECT *locked, const RECT *rect, unsigned int flags);
	HRESULT UnlockRect(unsigned int level);
	HRESULT GetLevelDesc(unsigned int level, D3DSURFACE_DESC *desc);

	// Creates the GL texture and the level surfaces on first use.
	void _createSurfacesIfNeeded();
};

class CubicTextureEmu : public BaseTextureEmu {
	LF_DECLARE_CLASS(CubicTextureEmu)
public:
	virtual D3DRESOURCETYPE GetType() { return D3DRTYPE_CUBETEXTURE; }
	static void unbindAll() {}

	HRESULT GetCubeMapSurface(D3DCUBEMAP_FACES face, unsigned int level, SurfaceEmu **surface);
	HRESULT LockRect(D3DCUBEMAP_FACES face, unsigned int level, D3DLOCKED_RECT *locked, const RECT *rect,
	                 unsigned int flags);
	HRESULT UnlockRect(D3DCUBEMAP_FACES face, unsigned int level);
	HRESULT GetLevelDesc(unsigned int level, D3DSURFACE_DESC *desc);

	void _createSurfacesIfNeeded();
};

// Volume textures are never available (CContextCaps says so).
class VolumeTextureEmu : public BaseTextureEmu {
	LF_DECLARE_CLASS(VolumeTextureEmu)
public:
	virtual D3DRESOURCETYPE GetType() { return D3DRTYPE_VOLUMETEXTURE; }
	static void unbindAll() {}
};

// ---------------------------------------------------------------------------
// Buffer storage: a VBO, or plain memory when VBOs are off.

class GLBufferImp {
public:
	virtual ~GLBufferImp() {}
	// Binds the buffer; returns the base to add offsets to (0 for a VBO).
	virtual char *bind() = 0;
	virtual void unbind() = 0;
	virtual void *lock(unsigned int flags) = 0;
	virtual void unlock() = 0;
	// Unbinds both buffer targets (if `unbindGL`) and makes the current device rebind its buffers.
	static void unbindAll(bool unbindGL);
};

class GLBufferImpGeneric : public GLBufferImp {
public:
	GLBufferImpGeneric(LData *data, unsigned int size);
	virtual ~GLBufferImpGeneric();
	virtual char *bind() { return (char *)_data->bytes(); }
	virtual void unbind() {}
	virtual void *lock(unsigned int) { return _data->bytes(); }
	virtual void unlock() {}

	LData *_data;
};

class GLBufferImpVBO : public GLBufferImp {
public:
	GLBufferImpVBO(LData *data, unsigned int size, GLenum target, unsigned int usage);
	virtual ~GLBufferImpVBO();
	virtual char *bind();
	virtual void unbind();
	virtual void *lock(unsigned int flags);
	virtual void unlock();

	GLuint _name;
	GLenum _target;
};

class VertexBufferEmu : public ResourceEmu {
	LF_DECLARE_CLASS(VertexBufferEmu)
public:
	VertexBufferEmu() : _length(0), _notifyDevice(true), _imp(0) {}
	virtual void dealloc();
	virtual D3DRESOURCETYPE GetType() { return D3DRTYPE_VERTEXBUFFER; }

	VertexBufferEmu *initWithParams(unsigned int length, unsigned int usage, D3DPOOL pool, DeviceEmu *device);
	HRESULT Lock(unsigned int offset, unsigned int size, void **data, unsigned int flags);
	HRESULT Unlock();
	HRESULT GetDesc(D3DVERTEXBUFFER_DESC *desc);
	char *bind();
	void unbind() { _imp->unbind(); }
	void _createBuffer();
	void _deleteBuffer();

	unsigned int _length;
	bool _notifyDevice;
	GLBufferImp *_imp;
};

class IndexBufferEmu : public ResourceEmu {
	LF_DECLARE_CLASS(IndexBufferEmu)
public:
	IndexBufferEmu() : _format(D3DFMT_INDEX16), _length(0), _notifyDevice(true), _imp(0) {}
	virtual void dealloc();
	virtual D3DRESOURCETYPE GetType() { return D3DRTYPE_INDEXBUFFER; }

	IndexBufferEmu *initWithParams(D3DFORMAT format, unsigned int length, unsigned int usage, D3DPOOL pool,
	                               DeviceEmu *device);
	HRESULT Lock(unsigned int offset, unsigned int size, void **data, unsigned int flags);
	HRESULT Unlock();
	HRESULT GetDesc(D3DINDEXBUFFER_DESC *desc);
	char *bind();
	void unbind() { _imp->unbind(); }
	void _createBuffer();
	void _deleteBuffer();

	D3DFORMAT _format;
	unsigned int _length;
	bool _notifyDevice;
	GLBufferImp *_imp;
};

class VertexDeclEmu : public IUnknownEmu {
	LF_DECLARE_CLASS(VertexDeclEmu)
public:
	VertexDeclEmu() : _device(0) {}
	VertexDeclEmu *initWithParams(const D3DVERTEXELEMENT9 *elements, DeviceEmu *device);
	// The elements, ending with D3DDECL_END().
	const D3DVERTEXELEMENT9 *vertexDeclaration() const { return &_elements[0]; }

	DeviceEmu *_device;
	std::vector<D3DVERTEXELEMENT9> _elements;
};

// An ARB vertex or fragment program.
class DeviceShaderEmu : public IUnknownEmu {
	LF_DECLARE_CLASS(DeviceShaderEmu)
public:
	DeviceShaderEmu() : _device(0), _program(0), _target(0) {}
	virtual void dealloc();
	DeviceShaderEmu *initWithParams(LString *source, GLenum target, DeviceEmu *device);
	// Binds and enables the program; returns its target.
	GLenum bind();
	GLenum target() const { return _target; }

	DeviceEmu *_device;
	GLuint _program;
	GLenum _target;
};

// A recorded list of device state changes (BeginStateBlock/EndStateBlock).
class StateBlockEmu : public IUnknownEmu {
	LF_DECLARE_CLASS(StateBlockEmu)
public:
	enum { kRenderState = 1, kTextureStageState, kSamplerState, kTexture, kStreamSource, kTransform,
	       kVSConstant, kPSConstant };
	struct SStateItemDescr {
		unsigned int type;
		unsigned int a, b, c;
	};

	StateBlockEmu();
	virtual void dealloc();
	StateBlockEmu *initWithParams(DeviceEmu *device);
	HRESULT Apply();

	void setRenderState(D3DRENDERSTATETYPE state, unsigned int value) { _add(kRenderState, state, value, 0); }
	void setTextureStageState(unsigned int stage, D3DTEXTURESTAGESTATETYPE type, unsigned int value)
	{
		_add(kTextureStageState, stage, type, value);
	}
	void setSamplerState(unsigned int stage, D3DSAMPLERSTATETYPE type, unsigned int value)
	{
		_add(kSamplerState, stage, type, value);
	}
	void setTexture(unsigned int stage, BaseTextureEmu *texture);
	void setStreamSource(unsigned int stream, VertexBufferEmu *vb, unsigned int offset, unsigned int stride);
	void setTransform(D3DTRANSFORMSTATETYPE type, const _SMatrix *m);
	void setVertexShaderConstantF(unsigned int start, const float *data, unsigned int count);
	void setPixelShaderConstantF(unsigned int start, const float *data, unsigned int count);
	void setIndices(IndexBufferEmu *ib);
	void setVertexDeclaration(VertexDeclEmu *decl);
	void setVertexShader(DeviceShaderEmu *shader);
	void setPixelShader(DeviceShaderEmu *shader);

	void _add(unsigned int type, unsigned int a, unsigned int b, unsigned int c)
	{
		SStateItemDescr d = { type, a, b, c };
		_items.push_back(d);
	}
	unsigned int _appendData(const void *data, unsigned int size);

	DeviceEmu *_device;
	bool _indicesSet, _declSet, _vsSet, _psSet;
	IndexBufferEmu *_indices;
	VertexBufferEmu *_streams[8];
	BaseTextureEmu *_textures[8];
	DeviceShaderEmu *_vs, *_ps;
	VertexDeclEmu *_decl;
	std::vector<char> _data;
	std::vector<SStateItemDescr> _items;
};

// D3DXCreateMesh's mesh: a vertex and an index buffer.
class DXMeshEmu : public IUnknownEmu {
	LF_DECLARE_CLASS(DXMeshEmu)
public:
	DXMeshEmu() : _vb(0), _ib(0), _numVertices(0), _numFaces(0) {}
	virtual void dealloc();
	DXMeshEmu *initWithParams(DeviceEmu *device, const D3DVERTEXELEMENT9 *decl, unsigned int numVertices,
	                          unsigned int numFaces, unsigned int options);
	HRESULT GetVertexBuffer(VertexBufferEmu **vb);
	HRESULT GetIndexBuffer(IndexBufferEmu **ib);
	unsigned int GetNumVertices() const { return _numVertices; }
	unsigned int GetNumFaces() const { return _numFaces; }

	VertexBufferEmu *_vb;
	IndexBufferEmu *_ib;
	unsigned int _numVertices, _numFaces;
};

// D3DXMESH option flags.
#define D3DXMESH_32BIT 0x001
#define D3DXMESH_VB_SYSTEMMEM 0x010
#define D3DXMESH_VB_MANAGED 0x020
#define D3DXMESH_VB_WRITEONLY 0x040
#define D3DXMESH_VB_DYNAMIC 0x080
#define D3DXMESH_VB_SOFTWARE 0x8000
#define D3DXMESH_IB_SYSTEMMEM 0x100
#define D3DXMESH_IB_MANAGED 0x200
#define D3DXMESH_IB_WRITEONLY 0x400
#define D3DXMESH_IB_DYNAMIC 0x800
#define D3DXMESH_IB_SOFTWARE 0x10000

// ---------------------------------------------------------------------------
// Device states as the device keeps them: each value with a "changed" flag.

struct SStateValue {
	unsigned int value;
	bool changed;
};
struct SRenderStates {
	SStateValue s[D3D_MAXRENDERSTATES];
};
struct STextureStates {
	SStateValue s[D3D_MAXTEXTURESTAGESTATES];
};
struct SSamplerStates {
	SStateValue s[D3D_MAXSAMPLERSTATES];
};

class DeviceStatesRemap {
public:
	static void applyRenderStates(SRenderStates *states);
	// Returns false when the stage's colour op is DISABLE.
	static bool applyTextureStates(STextureStates *states, DeviceEmu *device);
	static void applySamplerStates(GLenum target, SSamplerStates *states, DeviceEmu *device);
};

class DeviceEmu : public IUnknownEmu {
	LF_DECLARE_CLASS(DeviceEmu)
public:
	enum { kMaxStreams = 8, kMaxTextures = 8, kMaxVSConsts = 256, kMaxPSConsts = 128 };

	DeviceEmu();
	virtual void dealloc();
	DeviceEmu *initWithParams(CContext *context);

	// Makes the context's GL context current; false if there is none.
	bool ValidateContext();
	static DeviceEmu *currentDevice() { return sCurrentDevice; }
	CContext *context() const { return _context; }
	CContextCaps *contextCaps() const;
	static void *allocTextureRangeZone(unsigned int) { return 0; }
	static void freeTextureRangeZone(void *, unsigned int) {}

	void _txtUnit0WasChanged() { _textureChanged[0] = true; }
	void _vbWasChanged(VertexBufferEmu *vb);
	void _ibWasChanged(IndexBufferEmu *ib)
	{
		if (_indices == ib)
			_indicesChanged = true;
	}
	void _resetCurrentBuffers();

	static HRESULT TestCooperativeLevel() { return D3D_OK; }
	HRESULT CreateTexture(unsigned int width, unsigned int height, unsigned int levels, unsigned int usage,
	                      D3DFORMAT format, D3DPOOL pool, Texture2DEmu **texture, void *sharedHandle = 0);
	HRESULT CreateCubeTexture(unsigned int edge, unsigned int levels, unsigned int usage, D3DFORMAT format,
	                          D3DPOOL pool, CubicTextureEmu **texture, void *sharedHandle = 0);
	HRESULT CreateVolumeTexture(unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, D3DFORMAT,
	                            D3DPOOL, VolumeTextureEmu **texture, void * = 0)
	{
		*texture = 0;
		return D3D_OK;
	}
	HRESULT CreateVertexBuffer(unsigned int length, unsigned int usage, unsigned int fvf, D3DPOOL pool,
	                           VertexBufferEmu **vb, void *sharedHandle = 0);
	HRESULT CreateIndexBuffer(unsigned int length, unsigned int usage, D3DFORMAT format, D3DPOOL pool,
	                          IndexBufferEmu **ib, void *sharedHandle = 0);
	HRESULT CreateVertexDeclaration(const D3DVERTEXELEMENT9 *elements, VertexDeclEmu **decl);

	HRESULT SetRenderState(D3DRENDERSTATETYPE state, unsigned int value);
	HRESULT SetTextureStageState(unsigned int stage, D3DTEXTURESTAGESTATETYPE type, unsigned int value);
	HRESULT SetSamplerState(unsigned int stage, D3DSAMPLERSTATETYPE type, unsigned int value);
	HRESULT SetTexture(unsigned int stage, BaseTextureEmu *texture);
	HRESULT SetVertexDeclaration(VertexDeclEmu *decl);
	HRESULT SetStreamSource(unsigned int stream, VertexBufferEmu *vb, unsigned int offset, unsigned int stride);
	HRESULT SetIndices(IndexBufferEmu *ib);
	HRESULT SetTransform(D3DTRANSFORMSTATETYPE type, const _SMatrix *m);
	HRESULT SetVertexShader(DeviceShaderEmu *shader);
	HRESULT SetVertexShaderConstantF(unsigned int start, const float *data, unsigned int count);
	HRESULT SetPixelShader(DeviceShaderEmu *shader);
	HRESULT SetPixelShaderConstantF(unsigned int start, const float *data, unsigned int count);
	HRESULT BeginStateBlock();
	HRESULT EndStateBlock(StateBlockEmu **block);
	bool isRecordingStateBlock() const { return _recording != 0; }

	HRESULT DrawPrimitive(D3DPRIMITIVETYPE type, unsigned int startVertex, unsigned int primCount);
	HRESULT DrawIndexedPrimitive(D3DPRIMITIVETYPE type, int baseVertexIndex, unsigned int minIndex,
	                             unsigned int numVertices, unsigned int startIndex, unsigned int primCount);
	HRESULT _bindDeviceObjects(D3DPRIMITIVETYPE type);

	static DeviceEmu *sCurrentDevice;

	VertexDeclEmu *_decl;
	VertexBufferEmu *_streams[kMaxStreams];
	unsigned int _strides[kMaxStreams];
	bool _streamChanged[kMaxStreams];
	bool _streamsChanged;
	IndexBufferEmu *_indices;
	bool _indicesChanged;
	BaseTextureEmu *_textures[kMaxTextures];
	bool _textureChanged[kMaxTextures];
	DeviceShaderEmu *_vs;
	bool _vsChanged;
	DeviceShaderEmu *_ps;
	bool _psChanged;
	float _vsConsts[kMaxVSConsts][4];
	bool _vsConstDirty[kMaxVSConsts];
	unsigned int _vsDirtyList[kMaxVSConsts];
	unsigned int _vsDirtyCount;
	float _psConsts[kMaxPSConsts][4];
	bool _psConstDirty[kMaxPSConsts];
	unsigned int _psDirtyList[kMaxPSConsts];
	unsigned int _psDirtyCount;
	unsigned int _maxVSConsts, _maxPSConsts;
	StateBlockEmu *_recording;
	SRenderStates _renderStates;
	bool _renderStatesChanged;
	STextureStates _textureStates[kMaxTextures];
	SSamplerStates _samplerStates[kMaxTextures];
	unsigned int _savedMipFilter[kMaxTextures];	// the mip filter asked for, while overridden
	bool _mipFilterApplied[kMaxTextures];	// GL has the asked-for mip filter
	CContext *_context;
	unsigned int _maxSimultTextures;
	_SMatrix _world, _view, _projection, _texture[kMaxTextures];
	GLenum _primMode;
	GLenum _indexType;
	char *_indexBase;
	unsigned int _indexSize;
	// Client arrays currently enabled.
	bool _vertexArrayOn, _normalArrayOn, _colorArrayOn, _secondaryColorArrayOn, _fogCoordArrayOn;
	bool _psizeAttribOn, _weightArrayOn, _blendIndicesAttribOn, _tangentAttribOn, _binormalAttribOn;
	bool _texCoordArrayOn[kMaxTextures];
};

// ---------------------------------------------------------------------------
// Lockers: lock a resource for their scope.

class StSurfaceLocker {
public:
	StSurfaceLocker(SurfaceEmu *surface, unsigned int flags);
	~StSurfaceLocker();
	void *bits() const { return _locked.pBits; }
	int pitch() const { return _locked.Pitch; }

	SurfaceEmu *_surface;
	D3DSURFACE_DESC _desc;
	D3DLOCKED_RECT _locked;
	unsigned int _bytesPerPixel;
};

class StVertexBufferLocker {
public:
	StVertexBufferLocker(VertexBufferEmu *vb, unsigned int flags);
	StVertexBufferLocker(DXMeshEmu *mesh, unsigned int flags);
	~StVertexBufferLocker();
	void init(VertexBufferEmu *vb, unsigned int flags);
	void *data() const { return _data; }

	VertexBufferEmu *_vb;
	D3DVERTEXBUFFER_DESC _desc;
	void *_data;
};

class StIndexBufferLocker {
public:
	StIndexBufferLocker(IndexBufferEmu *ib, unsigned int flags);
	StIndexBufferLocker(DXMeshEmu *mesh, unsigned int flags);
	~StIndexBufferLocker();
	void init(IndexBufferEmu *ib, unsigned int flags);
	void *data() const { return _data; }

	IndexBufferEmu *_ib;
	D3DINDEXBUFFER_DESC _desc;
	void *_data;
};

// ---------------------------------------------------------------------------
// What the GL context can do, limited by FishHook's config.plist.

class CapsGlobalConfig : public LCleanupTask {
public:
	CapsGlobalConfig();
	virtual void performCleanup();
	static void initConfigMapIfNeeded();

	static LMap *_configMap;
	static int maxTUs;
	static bool volTexturesSupport;
	static int maxVSVer, maxPSVer, maxVPVer, maxFPVer;
	static bool useVBO, usePBO, useMPEngine, vSyncEnabled;
};

class CContextCaps {
public:
	CContextCaps(CContext *context);

	bool VBOsAreSupported() const { return _vbo && CapsGlobalConfig::useVBO; }
	bool PBOsAreSupported() const { return _pbo && CapsGlobalConfig::usePBO; }
	bool textureRectangleIsSupported() const { return _texRect; }
	bool textureRangeIsSupported() const { return _texRange; }
	bool generateMipmapIsSupported() const { return _genMipmap; }
	bool textureLodBiasIsSupported() const { return _lodBias; }
	bool textureEnvDot3IsSupported() const { return _dot3; }
	bool CompressedTexturesAreSupported() const { return _s3tc; }
	bool VolumeTexturesAreSupported() const { return false; }
	bool DepthTexturesAreSupported() const { return false; }
	bool dynamicTexturesAreSupported() const { return false; }
	unsigned int GetMaxSimultTextures() const;
	unsigned int GetMaxClipPlanesCount() const { return _maxClipPlanes; }
	unsigned int GetMaxVertexIndex() const { return 0xffff; }
	unsigned int GetMaxVSConstantIndex() const { return _maxVSConst; }
	unsigned int GetMaxPSConstantIndex() const { return _maxPSConst; }
	int GetVertexShadersVer() const { return _vsVer; }
	int GetPixelShadersVer() const { return _psVer; }
	unsigned int GetMaxTextureSize() const { return _maxTextureSize; }
	static bool VSyncEnabled();
	static bool MPEngineEnabled();

	bool _vbo, _pbo, _texRect, _texRange, _genMipmap, _lodBias, _dot3, _s3tc;
	unsigned int _maxTextureSize;
	unsigned int _maxClipPlanes;
	unsigned int _maxVSConst, _maxPSConst;
	int _vsVer, _psVer;
	unsigned int _maxTUs;
};

// An NSOpenGLContext kept for the life of the object.
class GLContextHolder : public LObject {
	LF_DECLARE_CLASS(GLContextHolder)
public:
	GLContextHolder();
	virtual void dealloc();
	void *glContext() const { return _context; }

	void *_context;	// NSOpenGLContext *
};

// ---------------------------------------------------------------------------
// D3DX-like helpers.

unsigned int FHGetSizeForDeclType(int type);
void FHGetGLParamsFromVertexDeclType(int type, GLenum *glType, GLint *count);
// The size of a vertex of `stream`: the end of its furthest element.
unsigned int FHGetDeclVertexSize(const D3DVERTEXELEMENT9 *decl, unsigned int stream);
HRESULT FHComputeBoundingBox(const Vector3 *first, unsigned int count, unsigned int stride, Vector3 *min,
                             Vector3 *max);
unsigned int FHGetSurfaceSize(unsigned int width, unsigned int height, D3DFORMAT format);
HRESULT FHFillTextureWithData(const D3DXIMAGE_INFO *info, const void *dds, Texture2DEmu *texture);
HRESULT FHGetImageInfoFromFileInMemory(const void *data, unsigned int size, D3DXIMAGE_INFO *info);
HRESULT FHCreateTextureFromFileInMemory(DeviceEmu *device, const void *data, unsigned int size, unsigned int unused,
                                        unsigned int usage, D3DPOOL pool, Texture2DEmu **texture);
HRESULT FHCreateMesh(unsigned int numFaces, unsigned int numVertices, unsigned int options,
                     const D3DVERTEXELEMENT9 *decl, DeviceEmu *device, DXMeshEmu **mesh);

#endif
