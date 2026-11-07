// Render profiles and shaders. A render profile (a .yaf file) holds a list
// of shaders; the first one the hardware can run is used. A shader has the
// vertex declaration, state blocks and its stages; each stage is an ARB
// vertex/fragment program pair with the constants it takes from the context,
// the render environment or the drawn model. Also the lights and clip planes
// of the render environment and the provider chains models answer through.
#ifndef FHRENDER_H
#define FHRENDER_H

#include "FHContext.h"
#include "LExpressionParser.h"
#include "LCleanupTask.h"

class CShader;
class CShaderStage;
class CShadersContainer;
class CDrawableModelData;

// Sine tables (512 steps a turn, 0x204 entries; the "cosine" table holds sines
// too), the identity matrix and an all-zero light.
extern const float *sSinfTable;
extern const float *sCosfTable;
extern const Matrix *sIdentityMatrixPtr;
extern const D3DLIGHT9 *sZeroLightPtr;

// The predefined render profiles, by name; their .yaf files are in the
// framework's resources.
extern LConstantString NoTextureRenderProfile;
extern LConstantString SimpleRenderProfile;
extern LConstantString BillboardRenderProfile;
extern LConstantString FontRenderProfile;
extern LConstantString PrimitivesProfile;

// Shader description keys.
extern LConstantString kShaderRequirements;
extern LConstantString kShaderActivateBlock;
extern LConstantString kShaderDeactivateBlock;
extern LConstantString kShaderIsTransparent;
extern LConstantString kShaderPSClipPlanesCount;
extern LConstantString kShaderNotClippable;
extern LConstantString kShaderForceSWVP;
extern LConstantString kShaderMaxLightsCount;
extern LConstantString kShaderCanDropShadow;
extern LConstantString kShaderDynamicStages;
extern LConstantString kShaderMaxBlendIndex;
extern LConstantString kShaderStages;
extern LConstantString kShaderStreams;
extern LConstantString kShaderDynamicBuffer;
extern LConstantString kShaderStreamIsInstancingStream;
extern LConstantString kShaderStreamDecl;
extern LConstantString kShaderStreamUsageKey;
extern LConstantString kShaderStreamTypeKey;
extern LConstantString kShaderStageActivateCondition;
extern LConstantString kShaderStageTextureSources;
extern LConstantString kShaderStageVertexShader;
extern LConstantString kShaderStagePixelShader;
extern LConstantString kShaderStageVSConstants;
extern LConstantString kShaderStagePSConstants;
// Variables of the requirement and condition expressions.
extern LConstantString kDevicePropClipPlanesCount;
extern LConstantString kDevicePropVSVersion;
extern LConstantString kDevicePropPSVersion;
extern LConstantString kDevicePropTSCount;
extern LConstantString kDevicePropVolTextures;
extern LConstantString kDevicePropDepthTextures;
extern LConstantString kEnvParamClipState;
extern LConstantString kEnvParamSMCount;
extern LConstantString kEnvParamSMUpdateMode;

// ---------------------------------------------------------------------------
// Shader constants. A stage's constants are described by register, source
// and update class; the sources are registered by name with
// ShaderConstantDef (the built-in ones are in CShaderStage::_dataPtrForConstant,
// the others are answered by models or by a data source object).

enum TShaderConstClass {
	kShaderConstStatic = 1,	// set once, recorded in the stage's state block
	kShaderConstEnvDepended = 2,	// set when the render environment changes
	kShaderConstDynamic = 3,	// set for every activation
	kShaderConstInstance = 4,	// per instance (vertex shaders only)
};

struct SShaderConstDescr;

// Answers the data of a constant registered with it.
class IShaderConstDS {
public:
	virtual const void *dataPtrForConstant(CDrawableModelData *data, const SShaderConstDescr *descr, Matrix *buffer) = 0;
	virtual ~IShaderConstDS() {}
};

struct SShaderConstDescr {
	int reg;	// the first register
	int cls;	// TShaderConstClass
	int count;	// registers
	IShaderConstDS *ds;
	int id;	// the constant source
	float value[4];	// a literal value (id kLiteralConstID)
};

enum { kLiteralConstID = 0x43 };

// What a constant source name was registered with.
struct SShaderConstInfo {
	int count;
	int cls;
	int id;
	IShaderConstDS *ds;
};

// Registers a constant source while it lives (usually a static).
class ShaderConstantDef {
public:
	ShaderConstantDef(int id, const char *name, int count, TShaderConstClass cls, IShaderConstDS *ds);
	~ShaderConstantDef();
	LString *_name;
	int _id;
};

// Registers a texture source name while it lives.
class TextureSourceDef {
public:
	TextureSourceDef(int id, const char *name);
	~TextureSourceDef();
	LString *_name;
};

class CShaderConstsArray : public TArray<SShaderConstDescr> {
};

// ---------------------------------------------------------------------------
// Provider chains: each link answers or hands over to the next.

class CConstDataProvider {
public:
	virtual const void *_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer) = 0;
	virtual CConstDataProvider *_nextConstDataProvider() = 0;
	virtual ~CConstDataProvider() {}
	// A constant with a data source asks it; the others go along the chain.
	const void *dataPtrForConstant(CDrawableModelData *data, const SShaderConstDescr *descr, Matrix *buffer);
};

class CInstanceConstDataProvider {
public:
	virtual const void *_instanceDataPtrForConstant(unsigned int id, void *buffer) = 0;
	virtual CInstanceConstDataProvider *_nextInstanceConstDataProvider() = 0;
	virtual ~CInstanceConstDataProvider() {}
	const void *instanceDataPtrForConstant(unsigned int id, void *buffer);
};

class CShaderStageStateProvider {
public:
	virtual CShaderStageStateProvider *_nextStageStateProvider() = 0;
	// Returns true when it decided.
	virtual bool _isStageOn(CShaderStage *, unsigned char *on)
	{
		*on = 1;
		return false;
	}
	virtual ~CShaderStageStateProvider() {}
	bool isStageOn(CShaderStage *stage);
};

class CTextureProviderCallback {
public:
	virtual BaseTextureEmu *provideTexture() = 0;
	virtual ~CTextureProviderCallback() {}
};

enum { kTextureSourcesCount = 10 };

class CDeviceTextureProvider {
public:
	CDeviceTextureProvider();
	virtual CDeviceTextureProvider *_nextTextureProvider() = 0;
	virtual bool _getTextureForSource(BaseTextureEmu **texture, int source);
	virtual ~CDeviceTextureProvider();
	BaseTextureEmu *textureForSource(int source);
	CTextureProviderCallback *textureProviderForSource(int source);
	// Callbacks that are LObjects are retained.
	void setTextureForSource(CTextureProviderCallback *callback, int source);

	CTextureProviderCallback *_sources[kTextureSourcesCount];
	LObject *_sourceOwners[kTextureSourcesCount];
};

class CClipPlanesProvider {
public:
	CClipPlanesProvider() : _planes(0) {}
	virtual void addClipPlane(CClipPlane *plane);
	virtual void removeClipPlane(CClipPlane *plane);
	virtual void removeAllClipPlanes();
	virtual CClipPlanesProvider *_nextClipPlanesProvider() = 0;
	virtual bool _getClipPlanes(CClipPlane ***planes, int *count);
	virtual ~CClipPlanesProvider();
	bool getClipPlanes(CClipPlane ***planes, int *count);

	LArray *_planes;
};

// A texture callback kept as an LObject.
class CTextureProvider : public LObject {
	LF_DECLARE_CLASS(CTextureProvider)
public:
	CTextureProvider() : _source(0), _sourceOwner(0) {}
	virtual void dealloc();
	CTextureProvider *initWithSource(CTextureProviderCallback *source);
	CTextureProviderCallback *source() const { return _source; }

	CTextureProviderCallback *_source;
	LObject *_sourceOwner;
};

// CDrawableModelData's provider interfaces (defined with that class).
LObject *FHModelDataObject(CDrawableModelData *data);
CConstDataProvider *FHModelDataConstProvider(CDrawableModelData *data);
CInstanceConstDataProvider *FHModelDataInstanceConstProvider(CDrawableModelData *data);
CShaderStageStateProvider *FHModelDataStageStateProvider(CDrawableModelData *data);

// ---------------------------------------------------------------------------
// Lights and clip planes.

class CClipPlane : public LObject {
	LF_DECLARE_CLASS(CClipPlane)
public:
	CClipPlane();
	virtual CClipPlane *init();
	CClipPlane *initWithPlane(const Plane *plane);
	const Plane *plane() const { return &_plane; }
	void setPlane(const Plane *plane) { _plane = *plane; }
	float priority() const { return _priority; }
	void setPriority(float priority) { _priority = priority; }

	Plane _plane;
	float _priority;
};

typedef D3DLIGHT9 SLightParams;

// A shadow map a light renders; none is made in this program.
class CShadowEnvMap : public LObject {
	LF_DECLARE_ABSTRACT_CLASS(CShadowEnvMap)
public:
	virtual void attachToLight(CContextLight *light) = 0;
	// From world space to the map's texture space.
	virtual const Matrix *textureMatrix() const = 0;
};

class CContextLight : public LObject, public CSceneStatePropertyOwner {
	LF_DECLARE_CLASS(CContextLight)
public:
	CContextLight();
	virtual void dealloc();
	virtual void enable();
	virtual void disable();
	virtual void enableSelf() {}
	virtual void disableSelf() {}
	virtual void storeSceneStateInStream(LData *stream, unsigned int flags);
	virtual const void *restoreSceneStateFromData(const void *data, unsigned int flags);

	CContextLight *initWithLightType(D3DLIGHTTYPE type);
	void setDirection(const Vector3 &direction);
	void attachShadowMap(CShadowEnvMap *map);
	CShadowEnvMap *shadowMap() const { return _shadowMap; }
	const SLightParams *params() const { return &_params; }
	bool isEnabled() const { return _enabled; }

	CShadowEnvMap *_shadowMap;
	// The stored state, in this order.
	SLightParams _params;
	float _intensity;
	int _enabled;
};

// Defers the render profiles' invalidation to the end of a block.
class StPostponeProfilesInvalidation {
public:
	StPostponeProfilesInvalidation(CRenderEnvironment *env);
	~StPostponeProfilesInvalidation();
	CRenderEnvironment *_env;
	bool _invalidated;
};

// ---------------------------------------------------------------------------
// Device shaders: ARB programs.

class CDeviceShader {
public:
	CDeviceShader(DeviceEmu *device) : _device(device), _code(0), _shader(0) {}
	virtual ~CDeviceShader();
	virtual GLenum bind();
	virtual GLenum glTarget() = 0;
	void setShaderCode(LString *code);
	DeviceShaderEmu *deviceShader() const { return _shader; }

	DeviceEmu *_device;
	LString *_code;
	DeviceShaderEmu *_shader;
};

class CVertexShader : public CDeviceShader {
public:
	CVertexShader(DeviceEmu *device, CDrawCache *drawCache) : CDeviceShader(device), _drawCache(drawCache) {}
	virtual GLenum bind();
	virtual GLenum glTarget() { return GL_VERTEX_PROGRAM_ARB; }
	void activateSelf();
	void activateImmediately(DeviceEmu *device);
	static void unbindAllExcept(GLenum target);
	CDrawCache *_drawCache;
};

class CPixelShader : public CDeviceShader {
public:
	CPixelShader(DeviceEmu *device, CDrawCache *drawCache) : CDeviceShader(device), _drawCache(drawCache) {}
	virtual GLenum bind();
	virtual GLenum glTarget() { return GL_FRAGMENT_PROGRAM_ARB; }
	void activateSelf();
	void activateImmediately(DeviceEmu *device);
	static void unbindAllExcept(GLenum target);
	CDrawCache *_drawCache;
};

// ---------------------------------------------------------------------------
// CShaderStage: one pass of a shader.

class CShaderStage : public CContextObject,
                     public CConstDataProvider,
                     public CInstanceConstDataProvider,
                     public CVarAddrCallback {
	LF_DECLARE_CLASS(CShaderStage)
public:
	CShaderStage();
	virtual void dealloc();
	virtual void processInitializationPair(const LObject *value, const LComparable *key, const LMap *map);
	virtual void instantiateSelf();
	virtual void releaseSelf();
	virtual void purgeSelf();
	virtual void restoreSelf();
	// Returns whether the stage is used in the environment.
	virtual bool adjustToNewEnvironment(const CRenderEnvironment *env);
	virtual bool checkConditionForEnv(const CRenderEnvironment *env);
	virtual void adjustEnvDependedVariables(const CRenderEnvironment *) {}
	virtual const int *addressOfVariable(LString *name);
	virtual bool validateInContext();
	virtual bool activateShaderStage();
	virtual bool reactivateShaderStage();
	virtual void deactivateShaderStage();
	virtual const void *_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer);
	virtual CConstDataProvider *_nextConstDataProvider();
	virtual const void *_instanceDataPtrForConstant(unsigned int, void *) { return 0; }
	virtual CInstanceConstDataProvider *_nextInstanceConstDataProvider();

	CShader *shader() const { return _shader; }
	CRenderProfile *renderProfile();
	bool isValid() const { return _valid; }
	int textureSourcesCount() const { return _textureSources ? _textureSources->count() : 0; }
	int textureSourceAtIndex(int i) const { return (*_textureSources)[i]; }
	const SShaderConstDescr *instanceVSConstantDescription();
	int instanceVSConstantsCount() const { return _vsInstance.count(); }
	int offsetForConstant(int id);
	CShadowEnvMap *shadowMapFromProfileAtIndex(long index);
	void setVSCode(const LString *code);
	void setPSCode(const LString *code);
	void setVSConstants(const SShaderConstDescr *items, long count);
	void setPSConstants(const SShaderConstDescr *items, long count);
	void addItemsFromArrayToArray(LArray *items, TArray<SShaderConstDescr> *out);
	void _resetClipSettings();
	void _updateEnvDependedConstantsIfNeeded(int *vsFirst, int *vsEnd, int *psFirst, int *psEnd);
	void _passConstantsToDevice(bool reactivation);

	static CSharableResource *_constMapMutex();
	static int textureSourceIDForKey(LString *key);
	static SShaderConstInfo shaderConstParamsForKey(LString *key);
	// The version in "<key>1.0"-like text as 0xMMmm, or 0.
	static unsigned int digitsSeparatedByDotInStringAfterKey(const LString *text, const LString *key);

	// Constant sources by name, their classes by id, texture sources by name.
	static LMap *sShaderConstsMap;
	static CHashTable *sShaderConstIDToConstClassMap;
	static LMap *sShaderTextureSrcsMap;

	CShader *_shader;
	bool _valid;
	long _index;	// in the shader's stages
	long _drawStageIndex;	// in the shader's active stages
	LString *_condition;
	LExpressionParser *_conditionParser;
	bool _usesVS, _usesPS;
	TArray<int> *_textureSources;
	LString *_vsCode, *_psCode;
	const CRenderEnvironment *_env;
	bool _clipInClipSpace;	// clip planes go to the vertex program's space
	bool _clipFixedFunction;
	CStateBlock *_activateBlock;	// also records the programs and static constants
	CStateBlock *_deactivateBlock;
	LData *_vsConsts;
	int _vsBase, _vsSize;	// registers
	LData *_psConsts;
	int _psBase, _psSize;
	CShaderConstsArray _vsStatic, _vsEnv, _vsDynamic, _vsInstance;
	CShaderConstsArray _psStatic, _psEnv, _psDynamic;
	bool _envConstsValid;
	int _objectID;
};

// ---------------------------------------------------------------------------
// CShader.

enum { kMaxShaderStreams = 8, kMaxDeclElements = 65 };

class CShader : public CContextObject, public CVarAddrCallback {
	LF_DECLARE_CLASS(CShader)
public:
	CShader();
	virtual void dealloc();
	virtual CShader *initWithMapAndContext(const LMap *map, void *context);
	virtual void processInitializationPair(const LObject *value, const LComparable *key, const LMap *map);
	virtual void instantiateSelf();
	virtual void releaseSelf();
	virtual void adjustToNewEnvironment(const CRenderEnvironment *env);
	// Returns the number of draw stages.
	virtual long activateShader(CDrawableModelData *data);
	virtual bool activateShaderStage(long stage);
	virtual void deactivateShader();
	virtual bool satisfiesRequirements();
	virtual const int *addressOfVariable(LString *name);
	virtual bool validateInContext();

	void reactivateShader(CDrawableModelData *data);
	void reloadShaderStages();
	void addShaderStage(CShaderStage *stage);
	void _buildVertexDeclWithArray(const LArray *streams);
	long drawStagesCount() const { return _activeStages->count(); }
	long shaderStageNumberForDrawStage(long stage) const { return (*_activeStages)[stage]->_index; }
	long usedTextureStagesCount() const;
	int textureIDForTextureStage(long stage) const { return _currentStage->textureSourceAtIndex(stage); }
	CShaderStage *currentStage() const { return _currentStage; }
	CDrawableModelData *currentModelData() const { return _modelData; }
	bool isValid() const { return _valid; }
	bool isTransparent() const { return _isTransparent; }
	const D3DVERTEXELEMENT9 *vertexDeclaration() const { return _decl; }

	static LMap *sUsageValuesMap;
	static LMap *sUsageIndexesMap;
	static LMap *sFormatValuesMap;
	static LMap *sDefaultFormatValuesMap;

	CShadersContainer *_container;
	bool _valid;
	LArray *_stages;
	TArray<CShaderStage *> *_activeStages;
	LString *_requirements;
	LExpressionParser *_requirementsParser;
	D3DVERTEXELEMENT9 _decl[kMaxDeclElements];
	VertexDeclEmu *_vertexDecl;
	CStateBlock *_activateBlock;
	CStateBlock *_deactivateBlock;
	bool _isTransparent;
	bool _forceSWVP;
	bool _dynamicStages;	// the model may switch stages off
	int _maxLightsCount;
	bool _canDropShadow;
	int _psClipPlanesCount;
	bool _notClippable;
	bool _dynamicBuffer[kMaxShaderStreams];
	bool _instancingStream[kMaxShaderStreams];
	int _maxTextureStages;
	int _maxBlendIndex;
	int _streamsCount;
	int _texcoordsCount;
	int _clipPlanesCount;
	bool _useSWVP;
	bool _clipPlanesSupported;
	CShaderStage *_currentStage;
	CDrawableModelData *_modelData;
};

class CShaderCleaner : public LCleanupTask {
public:
	virtual void performCleanup();
};

// The shaders of a profile, best first.
class CShadersContainer : public LObject {
	LF_DECLARE_CLASS(CShadersContainer)
public:
	CShadersContainer();
	virtual void dealloc();
	CShadersContainer *initWithDescrArray(LArray *shaders, CRenderProfile *profile, CHolder *holder);
	CShader *firstValidShader();
	void adjustToNewEnvironment(CRenderEnvironment *env);

	LArray *_shaders;
	CRenderProfile *_profile;
};

// ---------------------------------------------------------------------------
// CRenderProfile.

enum TInitDataType {
	kInitDataText = 0,	// a text representation of the shaders array
	kInitDataArchive = 1,	// an archived shaders array
};

class CRenderProfile : public CContextObject {
	LF_DECLARE_CLASS(CRenderProfile)
public:
	CRenderProfile();
	virtual void dealloc();
	virtual void instantiateSelf();
	virtual void releaseSelf();

	CRenderProfile *initWithHolderAndData(CHolder *holder, LData *data, TInitDataType type, const LString *path);
	LString *fileDescription() const { return _fileDescription; }
	LString *name() const { return _name; }
	CShader *shader() const { return _shader; }
	bool isTransparent() const { return _isTransparent; }

	// Returns the number of draw stages, 0 if the profile can't draw.
	long activateProfile(CDrawableModelData *data);
	void reactivateProfile(CDrawableModelData *data);
	void deactivateProfile() {}
	void performDeactivation();
	bool activateDrawStage(long stage);
	bool reactivateDrawStage(long stage);
	void invalEnvSettings();
	const D3DVERTEXELEMENT9 *vertexDeclaration();
	// The offset of an element in its stream, or ~0.
	unsigned int drawableElementOffset(D3DDECLUSAGE usage, unsigned char usageIndex, unsigned short *stream);
	unsigned int vertexStructSize(int stream);

	// A predefined profile name or a .yaf path.
	static CRenderProfile *renderProfileByFileDesc(LString *name, CHolder *holder);
	static CRenderProfile *renderProfileFromBundle(LString *resource, CFBundleRef bundle, CHolder *holder);

	static LMap *sPredefinedProfiles;

	long _activeStage;
	CShadersContainer *_container;
	LString *_fileDescription;
	LString *_name;
	CShader *_shader;
	CRenderEnvironment *_env;	// set while the environment settings hold
	bool _isTransparent;
};

#endif
