// CRenderEnvironment with its lights and clip planes, the context caps and
// their config, the ARB device shaders and the common static tables.

#include "FHRender.h"
#include "LScanner.h"
#include <OpenGL/glu.h>

LF_IMPLEMENT_CLASS(CClipPlane, 0)
LF_IMPLEMENT_CLASS(CContextLight, 0)

// ---------------------------------------------------------------------------
// Common statics: sine tables of 512 steps a turn (the "cosine" table holds
// sines as well, as in the original), the identity matrix and a zero light.

static float _sSinfTableContainer[0x204];
static float _sCosfTableContainer[0x204];
static Matrix _sOneMatrix;
static D3DLIGHT9 _sZeroLight;
const float *sSinfTable = _sSinfTableContainer;
const float *sCosfTable = _sCosfTableContainer;
const Matrix *sIdentityMatrixPtr = &_sOneMatrix;
const D3DLIGHT9 *sZeroLightPtr = &_sZeroLight;

class CCommonStaticsInitializer {
public:
	CCommonStaticsInitializer()
	{
		for (int i = 0; i < 0x204; i++) {
			float angle = (float)i * 6.2831855f * 0.001953125f;
			_sSinfTableContainer[i] = sinf(angle);
			_sCosfTableContainer[i] = sinf(angle);
		}
		_sOneMatrix.setIdentity();
		memset(&_sZeroLight, 0, sizeof _sZeroLight);
	}
};
static CCommonStaticsInitializer _sCommonStaticsInitializer;

// ---------------------------------------------------------------------------
// CClipPlane

CClipPlane::CClipPlane() : _priority(0)
{
	_plane.a = 0;
	_plane.b = 0;
	_plane.c = 1.0f;
	_plane.d = 0;
}

CClipPlane *CClipPlane::init()
{
	if (LObject::init() == 0)
		return 0;
	_plane.a = 0;
	_plane.b = 0;
	_plane.c = 1.0f;
	_plane.d = 0;
	return this;
}

CClipPlane *CClipPlane::initWithPlane(const Plane *plane)
{
	if (LObject::init() == 0)
		return 0;
	_plane = *plane;
	return this;
}

// ---------------------------------------------------------------------------
// CContextLight

CContextLight::CContextLight() : _shadowMap(0), _intensity(1.0f), _enabled(0)
{
	memset(&_params, 0, sizeof _params);
}

void CContextLight::dealloc()
{
	if (_shadowMap) {
		_shadowMap->attachToLight(0);
		_shadowMap->release();
		_shadowMap = 0;
	}
	LObject::dealloc();
}

void CContextLight::attachShadowMap(CShadowEnvMap *map)
{
	if (map) {
		map->retain();
		map->attachToLight(this);
	}
	if (_shadowMap) {
		_shadowMap->release();
		_shadowMap = 0;
	}
	_shadowMap = map;
}

void CContextLight::enable()
{
	if (!_enabled) {
		_enabled = 1;
		enableSelf();
	}
}

void CContextLight::disable()
{
	if (_enabled) {
		disableSelf();
		_enabled = 0;
	}
}

void CContextLight::setDirection(const Vector3 &direction)
{
	FHVec3Normalize((_SVector3 *)&_params.Direction, &direction);
}

CContextLight *CContextLight::initWithLightType(D3DLIGHTTYPE type)
{
	_params.Type = type;
	_params.Diffuse.r = _params.Diffuse.g = _params.Diffuse.b = _params.Diffuse.a = 1.0f;
	_params.Specular.r = _params.Specular.g = _params.Specular.b = 0;
	_params.Specular.a = 1.0f;
	return this;
}

// The parameters, intensity and enabled flag, little-endian as stored.
void CContextLight::storeSceneStateInStream(LData *stream, unsigned int)
{
	stream->appendBytes(&_params, 0x70);
}

const void *CContextLight::restoreSceneStateFromData(const void *data, unsigned int)
{
	memcpy(&_params, data, 0x70);
	return (const char *)data + 0x70;
}

// ---------------------------------------------------------------------------
// CRenderEnvironment

CRenderEnvironment::CRenderEnvironment(CContext *context)
    : _context(context), _usedProfiles(0), _activeProfile(0), _clipPlanesCount(0), _shadowMapUpdateMode(0),
      _shadowMapsCount(0), _envMap(0)
{
	_ambient = Color(0, 0, 0, 1.0f);
	_lights = LArray::_alloc()->init();
	_clipPlanes = LArray::_alloc()->init();
	_postponers = new TArray<StPostponeProfilesInvalidation *>;
}

CRenderEnvironment::~CRenderEnvironment()
{
	LRelease(_lights);
	_lights = 0;
	LRelease(_clipPlanes);
	_clipPlanes = 0;
	LRelease(_usedProfiles);
	_usedProfiles = 0;
	delete _postponers;
	_postponers = 0;
}

void CRenderEnvironment::endFrame()
{
	if (_activeProfile) {
		_activeProfile->performDeactivation();
		_activeProfile = 0;
	}
	LRelease(_usedProfiles);
	_usedProfiles = 0;
}

void CRenderEnvironment::beginFrameWithUsedProfiles(LSet *profiles)
{
	LRelease(_usedProfiles);
	_usedProfiles = 0;
	_usedProfiles = LRetain(profiles->allObjects());
	invalidateProfile(0);
}

void CRenderEnvironment::invalidateProfile(CRenderProfile *profile)
{
	for (unsigned int i = 0; i < _postponers->count(); i++)
		(*_postponers)[i]->_invalidated = true;
	if (profile) {
		profile->invalEnvSettings();
		return;
	}
	int count = _usedProfiles ? _usedProfiles->count() : 0;
	for (int i = 0; i < count; i++) {
		CRenderProfile *p = lobj_cast<CRenderProfile>(_usedProfiles->objectAtIndex(i));
		if (p)
			p->invalEnvSettings();
	}
}

void CRenderEnvironment::addLight(CContextLight *light, CRenderProfile *profile)
{
	_lights->addObject(light);
	_ambient.r += light->_params.Ambient.r;
	_ambient.g += light->_params.Ambient.g;
	_ambient.b += light->_params.Ambient.b;
	_ambient.a += light->_params.Ambient.a;
	if (_ambient.r > 1.0f)
		_ambient.r = 1.0f;
	if (_ambient.g > 1.0f)
		_ambient.g = 1.0f;
	if (_ambient.b > 1.0f)
		_ambient.b = 1.0f;
	_ambient.a = 0;
	invalidateProfile(profile);
}

void CRenderEnvironment::removeLight(CContextLight *light, CRenderProfile *profile)
{
	_lights->removeObject(light);
	invalidateProfile(profile);
}

void CRenderEnvironment::removeAllLights()
{
	_lights->removeAllObjects();
	_ambient = Color(0, 0, 0, 0);
	invalidateProfile(0);
}

void CRenderEnvironment::addClipPlane(CClipPlane *plane, CRenderProfile *profile)
{
	_clipPlanes->addObject(plane);
	_clipPlanesCount++;
	invalidateProfile(profile);
}

void CRenderEnvironment::removeClipPlane(CClipPlane *plane, CRenderProfile *profile)
{
	_clipPlanes->removeObject(plane);
	_clipPlanesCount--;
	invalidateProfile(profile);
}

void CRenderEnvironment::removeAllClipPlanes()
{
	_clipPlanes->removeAllObjects();
	_clipPlanesCount = 0;
	invalidateProfile(0);
}

void CRenderEnvironment::setCurrentEnvMap(CEnvironmentMap *map)
{
	_envMap = map;
	invalidateProfile(0);
}

const D3DLIGHT9 *CRenderEnvironment::environmentLightAtIndex(int index) const
{
	if (index >= 0 && index < (int)_lights->count())
		return &((CContextLight *)_lights->objectAtIndex(index))->_params;
	return sZeroLightPtr;
}

// The index-th shadow map of the lights that have one.
CShadowEnvMap *CRenderEnvironment::shadowMapAtIndex(long index)
{
	for (unsigned int i = 0; i < _lights->count(); i++) {
		CShadowEnvMap *map = ((CContextLight *)_lights->objectAtIndex(i))->_shadowMap;
		if (map) {
			if (index < 1)
				return map;
			index--;
		}
	}
	return 0;
}

StPostponeProfilesInvalidation::StPostponeProfilesInvalidation(CRenderEnvironment *env) : _env(env), _invalidated(false)
{
	_env->_postponers->addObject(this);
}

StPostponeProfilesInvalidation::~StPostponeProfilesInvalidation()
{
	if (_invalidated)
		_env->invalidateProfile(0);
	_env->_postponers->removeObject(this);
}

// ---------------------------------------------------------------------------
// CapsGlobalConfig: FishHook's config.plist, read once.

LMap *CapsGlobalConfig::_configMap = 0;
int CapsGlobalConfig::maxTUs = 2;
bool CapsGlobalConfig::volTexturesSupport = false;
int CapsGlobalConfig::maxVSVer = 0x101;
int CapsGlobalConfig::maxPSVer = 0;
int CapsGlobalConfig::maxVPVer = 0x100;
int CapsGlobalConfig::maxFPVer = 0;
bool CapsGlobalConfig::useVBO = true;
bool CapsGlobalConfig::usePBO = true;
bool CapsGlobalConfig::useMPEngine = false;
bool CapsGlobalConfig::vSyncEnabled = false;
static CapsGlobalConfig _sCapsGlobalConfig;

CapsGlobalConfig::CapsGlobalConfig() {}

void CapsGlobalConfig::performCleanup()
{
	LRelease(_configMap);
	_configMap = 0;
}

// "1.3" -> 0x103
static int _versionFromObject(LObject *object)
{
	float f = UMapParsingUtils::loadFloatFromObject(object) + 0.001f;
	return (int)f * 0x100 + (int)((f - floorf(f)) * 10.0f);
}

void CapsGlobalConfig::initConfigMapIfNeeded()
{
	if (_configMap)
		return;
	LData *data = LData::dataWithResource(LString::stringWithCString("config.plist"), 0, getFishHookModule());
	LMap *map = data ? lobj_cast<LMap>(data->propertyList()) : 0;
	_configMap = LRetain(map);
	if (_configMap == 0)
		return;
	LObject *value;
	if ((value = _configMap->objectForKey(LString::stringWithCString("MaxTextureUnits"))) != 0)
		maxTUs = UMapParsingUtils::loadIntFromObject(value);
	if ((value = _configMap->objectForKey(LString::stringWithCString("VolumeTexturesSupport"))) != 0)
		volTexturesSupport = UMapParsingUtils::loadIntFromObject(value);
	if ((value = _configMap->objectForKey(LString::stringWithCString("MaxVSVersion"))) != 0)
		maxVSVer = _versionFromObject(value);
	if ((value = _configMap->objectForKey(LString::stringWithCString("MaxPSVersion"))) != 0)
		maxPSVer = _versionFromObject(value);
	if ((value = _configMap->objectForKey(LString::stringWithCString("MaxVPVersion"))) != 0)
		maxVPVer = _versionFromObject(value);
	if ((value = _configMap->objectForKey(LString::stringWithCString("MaxFPVersion"))) != 0)
		maxFPVer = _versionFromObject(value);
	if ((value = _configMap->objectForKey(LString::stringWithCString("UseVBO"))) != 0)
		useVBO = UMapParsingUtils::loadIntFromObject(value);
	if ((value = _configMap->objectForKey(LString::stringWithCString("UsePBO"))) != 0)
		usePBO = UMapParsingUtils::loadIntFromObject(value);
	if ((value = _configMap->objectForKey(LString::stringWithCString("UseMPEngine"))) != 0)
		useMPEngine = UMapParsingUtils::loadIntFromObject(value);
	if ((value = _configMap->objectForKey(LString::stringWithCString("VSync"))) != 0)
		vSyncEnabled = UMapParsingUtils::loadIntFromObject(value);
}

// ---------------------------------------------------------------------------
// CContextCaps

CContextCaps::CContextCaps(CContext *context)
{
	CapsGlobalConfig::initConfigMapIfNeeded();
	DeviceEmu *device = context ? context->device() : 0;
	LF_CHECK_PTR(device);
	device->ValidateContext();
	_maxClipPlanes = 6;
	_maxTUs = 2;
	_maxTextureSize = 0x400;
	_vbo = _pbo = _texRect = _texRange = false;
	_genMipmap = _lodBias = _dot3 = _s3tc = false;
	_vsVer = _psVer = 0;
	_maxVSConst = _maxPSConst = 0;

	GLint value;
	glGetIntegerv(GL_MAX_TEXTURE_UNITS, &value);
	_maxTUs = value;
	glGetIntegerv(GL_MAX_CLIP_PLANES, &value);
	_maxClipPlanes = value;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &value);
	_maxTextureSize = value;
	const GLubyte *extensions = glGetString(GL_EXTENSIONS);
	_texRect = gluCheckExtension((const GLubyte *)"GL_EXT_texture_rectangle", extensions);
	_texRange = gluCheckExtension((const GLubyte *)"GL_APPLE_texture_range", extensions);
	_vbo = gluCheckExtension((const GLubyte *)"GL_ARB_vertex_buffer_object", extensions);
	_pbo = gluCheckExtension((const GLubyte *)"GL_ARB_pixel_buffer_object", extensions);
	_genMipmap = gluCheckExtension((const GLubyte *)"GL_SGIS_generate_mipmap", extensions);
	_lodBias = gluCheckExtension((const GLubyte *)"GL_EXT_texture_lod_bias", extensions);
	_dot3 = gluCheckExtension((const GLubyte *)"GL_ARB_texture_env_dot3", extensions);
	_s3tc = gluCheckExtension((const GLubyte *)"GL_EXT_texture_compression_s3tc", extensions);
	if (gluCheckExtension((const GLubyte *)"GL_ARB_vertex_program", extensions)) {
		GLint locals, temporaries, instructions, addressRegs;
		glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB, &locals);
		_maxVSConst = locals;
		glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_MAX_PROGRAM_TEMPORARIES_ARB, &temporaries);
		glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_MAX_PROGRAM_INSTRUCTIONS_ARB, &instructions);
		glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_MAX_PROGRAM_ADDRESS_REGISTERS_ARB, &addressRegs);
		_vsVer = (temporaries < 12 || instructions < 0x100 || _maxVSConst < 0x100 || addressRegs < 1) ? 0x101 : 0x200;
	}
	if (gluCheckExtension((const GLubyte *)"GL_ARB_fragment_program", extensions)) {
		GLint locals, temporaries, alu, tex;
		glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB, &locals);
		_maxPSConst = locals;
		glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_TEMPORARIES_ARB, &temporaries);
		glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_ALU_INSTRUCTIONS_ARB, &alu);
		glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_TEX_INSTRUCTIONS_ARB, &tex);
		_psVer = (temporaries < 12 || alu < 0x40 || tex < 0x20 || _maxPSConst < 0x20) ? 0x103 : 0x200;
	}
}

unsigned int CContextCaps::GetMaxSimultTextures() const
{
	return _maxTUs < (unsigned int)CapsGlobalConfig::maxTUs ? _maxTUs : (unsigned int)CapsGlobalConfig::maxTUs;
}

bool CContextCaps::VSyncEnabled()
{
	CapsGlobalConfig::initConfigMapIfNeeded();
	return CapsGlobalConfig::vSyncEnabled;
}

bool CContextCaps::MPEngineEnabled()
{
	CapsGlobalConfig::initConfigMapIfNeeded();
	return CapsGlobalConfig::useMPEngine;
}

// ---------------------------------------------------------------------------
// Device shaders

CDeviceShader::~CDeviceShader()
{
	LRelease(_code);
	_code = 0;
	FHSafeRelease(_shader);
}

GLenum CDeviceShader::bind()
{
	return _shader ? _shader->bind() : 0;
}

void CDeviceShader::setShaderCode(LString *code)
{
	LRelease(_code);
	_code = LString::_alloc()->init();
	_code->setValue(code);
	FHSafeRelease(_shader);
	_shader = DeviceShaderEmu::_alloc()->initWithParams(_code, glTarget(), _device);
}

GLenum CVertexShader::bind()
{
	GLenum target = _shader ? _shader->bind() : 0;
	unbindAllExcept(target);
	return target;
}

void CVertexShader::unbindAllExcept(GLenum target)
{
	if (target != GL_VERTEX_PROGRAM_ARB) {
		glBindProgramARB(GL_VERTEX_PROGRAM_ARB, 0);
		glDisable(GL_VERTEX_PROGRAM_ARB);
	}
}

void CVertexShader::activateSelf()
{
	_drawCache->setVertexShader(_shader);
}

void CVertexShader::activateImmediately(DeviceEmu *device)
{
	device->SetVertexShader(_shader);
}

GLenum CPixelShader::bind()
{
	GLenum target = _shader ? _shader->bind() : 0;
	unbindAllExcept(target);
	return target;
}

void CPixelShader::unbindAllExcept(GLenum target)
{
	if (target != GL_FRAGMENT_PROGRAM_ARB) {
		glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, 0);
		glDisable(GL_FRAGMENT_PROGRAM_ARB);
	}
}

void CPixelShader::activateSelf()
{
	_drawCache->setPixelShader(_shader);
}

void CPixelShader::activateImmediately(DeviceEmu *device)
{
	device->SetPixelShader(_shader);
}
