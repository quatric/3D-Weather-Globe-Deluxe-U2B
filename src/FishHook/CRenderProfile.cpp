// Render profiles and their shader containers.

#include "FHRender.h"
#include "LCoder.h"

LF_IMPLEMENT_CLASS(CRenderProfile, LF_CLASSOF(CContextObject))
LF_IMPLEMENT_CLASS(CShadersContainer, 0)

LConstantString NoTextureRenderProfile("NoTexture");
LConstantString SimpleRenderProfile("Simple");
LConstantString BillboardRenderProfile("Billboard");
LConstantString FontRenderProfile("Font");
LConstantString PrimitivesProfile("2DPrimitives");
static LConstantString _sNoTextureYaf("NoTexture.yaf");
static LConstantString _sSimpleYaf("Simple.yaf");
static LConstantString _sBillboardYaf("Billboard.yaf");
static LConstantString _sFontYaf("font.yaf");
static LConstantString _sPrimsYaf("2DPrimitives.yaf");

LMap *CRenderProfile::sPredefinedProfiles = 0;

// The profile names and their resources.
static LMap *_predefinedProfiles()
{
	if (!CRenderProfile::sPredefinedProfiles) {
		LObject *resources[] = {&_sNoTextureYaf, &_sSimpleYaf, &_sBillboardYaf, &_sFontYaf, &_sPrimsYaf};
		const LComparable *names[] = {&NoTextureRenderProfile, &SimpleRenderProfile, &BillboardRenderProfile,
		                              &FontRenderProfile, &PrimitivesProfile};
		CRenderProfile::sPredefinedProfiles = LMap::_alloc()->initWithObjectsAndKeys(resources, names, 5);
	}
	return CRenderProfile::sPredefinedProfiles;
}

// ---------------------------------------------------------------------------

CShadersContainer::CShadersContainer() : _shaders(LArray::_alloc()->init()), _profile(0)
{
}

void CShadersContainer::dealloc()
{
	LRelease(_shaders);
	_shaders = 0;
	LObject::dealloc();
}

CShadersContainer *CShadersContainer::initWithDescrArray(LArray *shaders, CRenderProfile *profile, CHolder *holder)
{
	_profile = profile;
	for (int i = 0; i < (int)shaders->count(); i++) {
		LMap *map = lobj_cast<LMap>(shaders->objectAtIndex(i));
		if (!map)
			LF_RAISE(NilPtrException());
		CShader *shader = lobj_cast<CShader>(CContextObject::contextObjectWithMap(map, holder));
		if (!shader)
			LF_RAISE(NilPtrException());
		shader->_container = this;
		_shaders->addObject(shader);
	}
	return this;
}

CShader *CShadersContainer::firstValidShader()
{
	for (int i = 0; i < (int)_shaders->count(); i++) {
		CShader *shader = (CShader *)_shaders->objectAtIndex(i);
		if (shader->isValid())
			return shader;
	}
	return 0;
}

void CShadersContainer::adjustToNewEnvironment(CRenderEnvironment *env)
{
	int n = _shaders->count();
	for (int i = 0; i < n; i++)
		((CShader *)_shaders->objectAtIndex(i))->adjustToNewEnvironment(env);
}

// ---------------------------------------------------------------------------

CRenderProfile::CRenderProfile()
    : CContextObject(4), _activeStage(-1), _container(0), _fileDescription(0), _name(0), _shader(0), _env(0),
      _isTransparent(false)
{
}

CRenderProfile *CRenderProfile::initWithHolderAndData(CHolder *holder, LData *data, TInitDataType type,
                                                      const LString *path)
{
	initWithHolder(holder);
	_fileDescription = LRetain(LString::stringWithString(path));
	LArray *shaders = 0;
	if (type == kInitDataText)
		shaders = LAutorelease(LArray::_alloc()->initWithTextRepresentation(data));
	else if (type == kInitDataArchive)
		shaders = lobj_cast<LArray>(LCoder::unarchiveRootObject(data));
	if (!shaders)
		LF_RAISE(NilPtrException());
	_container = CShadersContainer::_alloc()->initWithDescrArray(shaders, this, holder);
	return this;
}

void CRenderProfile::dealloc()
{
	LRelease(_container);
	_container = 0;
	if (_holder) {
		CRenderEnvironment *env = _context ? _context->renderEnvironment() : 0;
		if (env && env->_activeProfile == this)
			env->_activeProfile = 0;
		_holder->removeRenderProfile(this);
	}
	LRelease(_fileDescription);
	_fileDescription = 0;
	LRelease(_name);
	_name = 0;
	CContextObject::dealloc();
}

void CRenderProfile::instantiateSelf()
{
	_shader = LRetain(_container->firstValidShader());
	if (_shader)
		_isTransparent = _shader->isTransparent();
}

void CRenderProfile::releaseSelf()
{
	LRelease(_shader);
	_shader = 0;
}

const D3DVERTEXELEMENT9 *CRenderProfile::vertexDeclaration()
{
	return _shader ? _shader->vertexDeclaration() : 0;
}

void CRenderProfile::invalEnvSettings()
{
	_env = 0;
	_activeStage = -1;
}

void CRenderProfile::performDeactivation()
{
	if (_shader)
		_shader->deactivateShader();
	_context->renderEnvironment()->_activeProfile = 0;
	_activeStage = -1;
}

long CRenderProfile::activateProfile(CDrawableModelData *data)
{
	if (!_shader || !_context)
		return 0;
	CRenderEnvironment *env = _context->renderEnvironment();
	if (!_env) {
		if (!env)
			LF_RAISE(NilPtrException());
		_env = env;
		_shader->adjustToNewEnvironment(env);
	}
	if (env->_activeProfile == this) {
		reactivateProfile(data);
		return _shader->drawStagesCount();
	}
	if (env->_activeProfile)
		env->_activeProfile->performDeactivation();
	env->_activeProfile = this;
	return _shader->activateShader(data);
}

void CRenderProfile::reactivateProfile(CDrawableModelData *data)
{
	_shader->reactivateShader(data);
}

bool CRenderProfile::activateDrawStage(long stage)
{
	if (_activeStage == stage)
		return _shader->currentStage()->reactivateShaderStage();
	bool ok = _shader->activateShaderStage(stage);
	_activeStage = stage;
	return ok;
}

bool CRenderProfile::reactivateDrawStage(long)
{
	return _shader->currentStage()->reactivateShaderStage();
}

unsigned int CRenderProfile::drawableElementOffset(D3DDECLUSAGE usage, unsigned char usageIndex, unsigned short *stream)
{
	const D3DVERTEXELEMENT9 *e = vertexDeclaration();
	if (e)
		for (; e->Type != D3DDECLTYPE_UNUSED; e++)
			if (e->Usage == usage && e->UsageIndex == usageIndex) {
				if (stream)
					*stream = e->Stream;
				return e->Offset;
			}
	return ~0u;
}

unsigned int CRenderProfile::vertexStructSize(int stream)
{
	return FHGetDeclVertexSize(vertexDeclaration(), stream);
}

CRenderProfile *CRenderProfile::renderProfileByFileDesc(LString *name, CHolder *holder)
{
	CRenderProfile *profile;
	LString *resource = (LString *)_predefinedProfiles()->objectForKey(name);
	if (resource)
		profile = renderProfileFromBundle(resource, getFishHookModule(), holder);
	else if (holder)
		profile = holder->renderProfileByFileDesc(name);
	else
		return 0;
	if (profile && !profile->_name) {
		profile->_name = LString::_alloc();
		profile->_name->setValue(name);
	}
	return profile;
}

CRenderProfile *CRenderProfile::renderProfileFromBundle(LString *resource, CFBundleRef bundle, CHolder *holder)
{
	CFURLRef url = CFBundleCopyResourceURL(bundle, resource->cfString(), 0, 0);
	if (!url)
		return 0;
	CFStringRef path = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
	LString *p = LString::stringWithCFString(path);
	CFRelease(path);
	CFRelease(url);
	return renderProfileByFileDesc(p, holder);
}
