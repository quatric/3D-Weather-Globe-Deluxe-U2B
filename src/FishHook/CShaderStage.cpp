// CShaderStage, the shader constant and texture source registries, the
// built-in constants and the provider chains.

#include "FHRender.h"
#include "LScanner.h"

LF_IMPLEMENT_CLASS(CShaderStage, LF_CLASSOF(CContextObject))
LF_IMPLEMENT_CLASS(CTextureProvider, 0)

LConstantString kShaderStageActivateCondition("Condition");
LConstantString kShaderStageTextureSources("TextureSources");
LConstantString kShaderStageVertexShader("VertexShader");
LConstantString kShaderStagePixelShader("PixelShader");
LConstantString kShaderStageVSConstants("VSConstants");
LConstantString kShaderStagePSConstants("PSConstants");
LConstantString kEnvParamClipState("ClippingIsActive");
LConstantString kEnvParamSMCount("ShadowMapsCount");
LConstantString kEnvParamSMUpdateMode("ShadowMapUpdateMode");
static LConstantString _sVSKey("!!ARBvp");
static LConstantString _sPSKey("!!ARBfp");

LMap *CShaderStage::sShaderConstsMap = 0;
CHashTable *CShaderStage::sShaderConstIDToConstClassMap = 0;
LMap *CShaderStage::sShaderTextureSrcsMap = 0;

CSharableResource *CShaderStage::_constMapMutex()
{
	static CSharableResource sConstMapMutex;
	return &sConstMapMutex;
}

// ---------------------------------------------------------------------------
// Registries.

ShaderConstantDef::ShaderConstantDef(int id, const char *name, int count, TShaderConstClass cls, IShaderConstDS *ds)
{
	StSharableLocker lock(CShaderStage::_constMapMutex());
	if (!CShaderStage::sShaderConstsMap) {
		CShaderStage::sShaderConstsMap = LMap::_alloc()->init();
		CShaderStage::sShaderConstIDToConstClassMap = new CHashTable;
	}
	_name = LString::alloc_with_new()->initWithCString(name);
	_id = id;
	SShaderConstInfo info = {count, cls, id, ds};
	LValue *value = LValue::_alloc();
	value->setValue(&info, sizeof info);
	CShaderStage::sShaderConstsMap->setObjectForKey(value, _name);
	CShaderStage::sShaderConstIDToConstClassMap->setValueForKey(cls, id);
	value->release();
}

ShaderConstantDef::~ShaderConstantDef()
{
	StSharableLocker lock(CShaderStage::_constMapMutex());
	CShaderStage::sShaderConstsMap->removeObjectForKey(_name);
	CShaderStage::sShaderConstIDToConstClassMap->removeValueForKey(_id);
	_name->release();
}

TextureSourceDef::TextureSourceDef(int id, const char *name)
{
	StSharableLocker lock(CShaderStage::_constMapMutex());
	if (!CShaderStage::sShaderTextureSrcsMap)
		CShaderStage::sShaderTextureSrcsMap = LMap::_alloc()->init();
	_name = LString::alloc_with_new()->initWithCString(name);
	LValue *value = LValue::_alloc();
	value->setValue(id);
	CShaderStage::sShaderTextureSrcsMap->setObjectForKey(value, _name);
	value->release();
}

TextureSourceDef::~TextureSourceDef()
{
	StSharableLocker lock(CShaderStage::_constMapMutex());
	CShaderStage::sShaderTextureSrcsMap->removeObjectForKey(_name);
	_name->release();
}

int CShaderStage::textureSourceIDForKey(LString *key)
{
	StSharableLocker lock(_constMapMutex());
	if (!sShaderTextureSrcsMap || !key)
		return -1;
	LValue *value = (LValue *)sShaderTextureSrcsMap->objectForKey(key);
	if (!value)
		value = (LValue *)sShaderTextureSrcsMap->objectForKey(key->lowercaseString());
	return value ? value->intValue() : -1;
}

SShaderConstInfo CShaderStage::shaderConstParamsForKey(LString *key)
{
	SShaderConstInfo info = {1, 0, -1, 0};
	StSharableLocker lock(_constMapMutex());
	if (!sShaderConstsMap || !key)
		return info;
	LValue *value = (LValue *)sShaderConstsMap->objectForKey(key);
	if (!value)
		value = (LValue *)sShaderConstsMap->objectForKey(key->lowercaseString());
	if (value)
		value->getValue(&info);
	return info;
}

// The built-in constants (answered by _dataPtrForConstant, or by the drawn
// model for the material and texture matrices) and texture sources.
static ShaderConstantDef _shaderConstDef_PI(0, "PI", 1, kShaderConstStatic, 0);
static ShaderConstantDef _shaderConstDef_SinCoeffs(1, "SinCoeffs", 1, kShaderConstStatic, 0);
static ShaderConstantDef _shaderConstDef_CosCoeffs(2, "CosCoeffs", 1, kShaderConstStatic, 0);
static ShaderConstantDef _shaderConstDef_WorldViewProjMtx(3, "WorldViewProjMtx", 4, kShaderConstDynamic, 0);
static ShaderConstantDef _shaderConstDef_InvWorldMtx(4, "InvWorldMtx", 3, kShaderConstDynamic, 0);
static ShaderConstantDef _shaderConstDef_WorldMtx(5, "WorldMtx", 4, kShaderConstDynamic, 0);
static ShaderConstantDef _shaderConstDef_ViewProjMtx(6, "ViewProjMtx", 4, kShaderConstEnvDepended, 0);
static ShaderConstantDef _shaderConstDef_ViewRotMtx(7, "ViewRotMtx", 3, kShaderConstEnvDepended, 0);
static ShaderConstantDef _shaderConstDef_ViewPortSize(8, "ViewPortSize", 1, kShaderConstEnvDepended, 0);
static ShaderConstantDef _shaderConstDef_MtlDiffuse(9, "MtlDiffuse", 1, kShaderConstDynamic, 0);
static ShaderConstantDef _shaderConstDef_ClipPlane0(10, "ClipPlane0", 1, kShaderConstEnvDepended, 0);
static ShaderConstantDef _shaderConstDef_ClipPlane1(11, "ClipPlane1", 1, kShaderConstEnvDepended, 0);
static ShaderConstantDef _shaderConstDef_ClipPlane2(12, "ClipPlane2", 1, kShaderConstEnvDepended, 0);
static ShaderConstantDef _shaderConstDef_ClipPlane3(13, "ClipPlane3", 1, kShaderConstEnvDepended, 0);
static ShaderConstantDef _shaderConstDef_ClipPlane4(14, "ClipPlane4", 1, kShaderConstEnvDepended, 0);
static ShaderConstantDef _shaderConstDef_ClipPlane5(15, "ClipPlane5", 1, kShaderConstEnvDepended, 0);
static ShaderConstantDef _shaderConstDef_ClipPlaneObjSpace0(16, "ClipPlaneObjSpace0", 1, kShaderConstDynamic, 0);
static ShaderConstantDef _shaderConstDef_ClipPlaneObjSpace1(17, "ClipPlaneObjSpace1", 1, kShaderConstDynamic, 0);
static ShaderConstantDef _shaderConstDef_ClipPlaneObjSpace2(18, "ClipPlaneObjSpace2", 1, kShaderConstDynamic, 0);
static ShaderConstantDef _shaderConstDef_ClipPlaneObjSpace3(19, "ClipPlaneObjSpace3", 1, kShaderConstDynamic, 0);
static ShaderConstantDef _shaderConstDef_ClipPlaneObjSpace4(20, "ClipPlaneObjSpace4", 1, kShaderConstDynamic, 0);
static ShaderConstantDef _shaderConstDef_ClipPlaneObjSpace5(21, "ClipPlaneObjSpace5", 1, kShaderConstDynamic, 0);
static ShaderConstantDef _shaderConstDef_EyePoint(22, "EyePoint", 1, kShaderConstEnvDepended, 0);
static ShaderConstantDef _shaderConstDef_EyePointObjSpace(23, "EyePointObjSpace", 1, kShaderConstDynamic, 0);
static ShaderConstantDef _shaderConstDef_LightAmbient(24, "LightAmbient", 1, kShaderConstEnvDepended, 0);
static ShaderConstantDef _shaderConstDef_LightDirection0(25, "LightDirection0", 1, kShaderConstEnvDepended, 0);
static ShaderConstantDef _shaderConstDef_LightDirectionObjSpace0(26, "LightDirectionObjSpace0", 1,
                                                                 kShaderConstDynamic, 0);
static ShaderConstantDef _shaderConstDef_LightDiffuse0(27, "LightDiffuse0", 1, kShaderConstEnvDepended, 0);
static ShaderConstantDef _shaderConstDef_LightSpecular0(28, "LightSpecular0", 1, kShaderConstEnvDepended, 0);
static ShaderConstantDef _shaderConstDef_LightDirection1(29, "LightDirection1", 1, kShaderConstEnvDepended, 0);
static ShaderConstantDef _shaderConstDef_LightDirectionObjSpace1(30, "LightDirectionObjSpace1", 1,
                                                                 kShaderConstDynamic, 0);
static ShaderConstantDef _shaderConstDef_LightDiffuse1(31, "LightDiffuse1", 1, kShaderConstEnvDepended, 0);
static ShaderConstantDef _shaderConstDef_LightSpecular1(32, "LightSpecular1", 1, kShaderConstEnvDepended, 0);
static ShaderConstantDef _shaderConstDef_LightDirection2(33, "LightDirection2", 1, kShaderConstEnvDepended, 0);
static ShaderConstantDef _shaderConstDef_LightDirectionObjSpace2(34, "LightDirectionObjSpace2", 1,
                                                                 kShaderConstDynamic, 0);
static ShaderConstantDef _shaderConstDef_LightDiffuse2(35, "LightDiffuse2", 1, kShaderConstEnvDepended, 0);
static ShaderConstantDef _shaderConstDef_LightDirection3(36, "LightDirection3", 1, kShaderConstEnvDepended, 0);
static ShaderConstantDef _shaderConstDef_LightDirectionObjSpace3(37, "LightDirectionObjSpace3", 1,
                                                                 kShaderConstDynamic, 0);
static ShaderConstantDef _shaderConstDef_LightDiffuse3(38, "LightDiffuse3", 1, kShaderConstEnvDepended, 0);
static ShaderConstantDef _shaderConstDef_TextureMatrix0Row0(39, "TextureMatrix0Row0", 1, kShaderConstDynamic, 0);
static ShaderConstantDef _shaderConstDef_TextureMatrix0Row1(40, "TextureMatrix0Row1", 1, kShaderConstDynamic, 0);
static ShaderConstantDef _shaderConstDef_TextureMatrix0Row2(41, "TextureMatrix0Row2", 1, kShaderConstDynamic, 0);
static ShaderConstantDef _shaderConstDef_TextureMatrix1Row0(42, "TextureMatrix1Row0", 1, kShaderConstDynamic, 0);
static ShaderConstantDef _shaderConstDef_TextureMatrix1Row1(43, "TextureMatrix1Row1", 1, kShaderConstDynamic, 0);
static ShaderConstantDef _shaderConstDef_TextureMatrix1Row2(44, "TextureMatrix1Row2", 1, kShaderConstDynamic, 0);
static ShaderConstantDef _shaderConstDef_TextureMatrix2Row0(45, "TextureMatrix2Row0", 1, kShaderConstDynamic, 0);
static ShaderConstantDef _shaderConstDef_TextureMatrix2Row1(46, "TextureMatrix2Row1", 1, kShaderConstDynamic, 0);
static ShaderConstantDef _shaderConstDef_TextureMatrix2Row2(47, "TextureMatrix2Row2", 1, kShaderConstDynamic, 0);
static ShaderConstantDef _shaderConstDef_TextureMatrix3Row0(48, "TextureMatrix3Row0", 1, kShaderConstDynamic, 0);
static ShaderConstantDef _shaderConstDef_TextureMatrix3Row1(49, "TextureMatrix3Row1", 1, kShaderConstDynamic, 0);
static ShaderConstantDef _shaderConstDef_TextureMatrix3Row2(50, "TextureMatrix3Row2", 1, kShaderConstDynamic, 0);
static ShaderConstantDef _shaderConstDef_TextureMatrix4Row0(51, "TextureMatrix4Row0", 1, kShaderConstDynamic, 0);
static ShaderConstantDef _shaderConstDef_TextureMatrix4Row1(52, "TextureMatrix4Row1", 1, kShaderConstDynamic, 0);
static ShaderConstantDef _shaderConstDef_TextureMatrix4Row2(53, "TextureMatrix4Row2", 1, kShaderConstDynamic, 0);
static ShaderConstantDef _shaderConstDef_TextureMatrix5Row0(54, "TextureMatrix5Row0", 1, kShaderConstDynamic, 0);
static ShaderConstantDef _shaderConstDef_TextureMatrix5Row1(55, "TextureMatrix5Row1", 1, kShaderConstDynamic, 0);
static ShaderConstantDef _shaderConstDef_TextureMatrix5Row2(56, "TextureMatrix5Row2", 1, kShaderConstDynamic, 0);
static ShaderConstantDef _shaderConstDef_TextureMatrix6Row0(57, "TextureMatrix6Row0", 1, kShaderConstDynamic, 0);
static ShaderConstantDef _shaderConstDef_TextureMatrix6Row1(58, "TextureMatrix6Row1", 1, kShaderConstDynamic, 0);
static ShaderConstantDef _shaderConstDef_TextureMatrix6Row2(59, "TextureMatrix6Row2", 1, kShaderConstDynamic, 0);
static ShaderConstantDef _shaderConstDef_TextureMatrix7Row0(60, "TextureMatrix7Row0", 1, kShaderConstDynamic, 0);
static ShaderConstantDef _shaderConstDef_TextureMatrix7Row1(61, "TextureMatrix7Row1", 1, kShaderConstDynamic, 0);
static ShaderConstantDef _shaderConstDef_TextureMatrix7Row2(62, "TextureMatrix7Row2", 1, kShaderConstDynamic, 0);
static ShaderConstantDef _shaderConstDef_ShadowMapObjToTexMtx0(63, "ShadowMapObjToTexMtx0", 3, kShaderConstDynamic, 0);
static ShaderConstantDef _shaderConstDef_ShadowMapObjToTexMtx1(64, "ShadowMapObjToTexMtx1", 3, kShaderConstDynamic, 0);
static ShaderConstantDef _shaderConstDef_FogColor(65, "FogColor", 1, kShaderConstEnvDepended, 0);
static ShaderConstantDef _shaderConstDef_FogParams(66, "FogParams", 1, kShaderConstDynamic, 0);

static TextureSourceDef _textureSrcDef_Diffuse0(0, "Diffuse0");
static TextureSourceDef _textureSrcDef_Diffuse1(1, "Diffuse1");
static TextureSourceDef _textureSrcDef_Diffuse2(2, "Diffuse2");
static TextureSourceDef _textureSrcDef_Diffuse3(3, "Diffuse3");
static TextureSourceDef _textureSrcDef_BumpMap(4, "BumpMap");
static TextureSourceDef _textureSrcDef_ReflectionMap(5, "ReflectionMap");
static TextureSourceDef _textureSrcDef_EnvironmentMap0(6, "EnvironmentMap0");
static TextureSourceDef _textureSrcDef_EnvironmentMap1(7, "EnvironmentMap1");
static TextureSourceDef _textureSrcDef_EnvironmentMap2(8, "EnvironmentMap2");
static TextureSourceDef _textureSrcDef_EnvironmentMap3(9, "EnvironmentMap3");
static TextureSourceDef _textureSrcDef_SpecularTable(11, "SpecularTable");
static TextureSourceDef _textureSrcDef_FresnelTable(12, "FresnelTable");

// ---------------------------------------------------------------------------
// Provider chains.

const void *CConstDataProvider::dataPtrForConstant(CDrawableModelData *data, const SShaderConstDescr *descr,
                                                   Matrix *buffer)
{
	if (descr->ds)
		return descr->ds->dataPtrForConstant(data, descr, buffer);
	for (CConstDataProvider *p = this; p; p = p->_nextConstDataProvider()) {
		const void *ptr = p->_dataPtrForConstant(descr, buffer);
		if (ptr)
			return ptr;
	}
	return 0;
}

const void *CInstanceConstDataProvider::instanceDataPtrForConstant(unsigned int id, void *buffer)
{
	for (CInstanceConstDataProvider *p = this; p; p = p->_nextInstanceConstDataProvider()) {
		const void *ptr = p->_instanceDataPtrForConstant(id, buffer);
		if (ptr)
			return ptr;
	}
	return 0;
}

bool CShaderStageStateProvider::isStageOn(CShaderStage *stage)
{
	unsigned char on = 1;
	for (CShaderStageStateProvider *p = this; p; p = p->_nextStageStateProvider())
		if (p->_isStageOn(stage, &on))
			break;
	return on != 0;
}

CDeviceTextureProvider::CDeviceTextureProvider()
{
	memset(_sources, 0, sizeof _sources);
	memset(_sourceOwners, 0, sizeof _sourceOwners);
}

CDeviceTextureProvider::~CDeviceTextureProvider()
{
	for (int i = 0; i < kTextureSourcesCount; i++) {
		LRelease(_sourceOwners[i]);
		_sourceOwners[i] = 0;
		_sources[i] = 0;
	}
}

BaseTextureEmu *CDeviceTextureProvider::textureForSource(int source)
{
	BaseTextureEmu *texture = 0;
	for (CDeviceTextureProvider *p = this; p; p = p->_nextTextureProvider())
		if (p->_getTextureForSource(&texture, source))
			break;
	return texture;
}

CTextureProviderCallback *CDeviceTextureProvider::textureProviderForSource(int source)
{
	return (unsigned int)source < kTextureSourcesCount ? _sources[source] : 0;
}

bool CDeviceTextureProvider::_getTextureForSource(BaseTextureEmu **texture, int source)
{
	if ((unsigned int)source >= kTextureSourcesCount || !_sources[source])
		return false;
	*texture = _sources[source]->provideTexture();
	return true;
}

void CDeviceTextureProvider::setTextureForSource(CTextureProviderCallback *callback, int source)
{
	if ((unsigned int)source >= kTextureSourcesCount)
		return;
	_sources[source] = callback;
	LObject *owner = LRetain(dynamic_cast<LObject *>(callback));
	LRelease(_sourceOwners[source]);
	_sourceOwners[source] = owner;
}

CClipPlanesProvider::~CClipPlanesProvider()
{
	LRelease(_planes);
	_planes = 0;
}

void CClipPlanesProvider::addClipPlane(CClipPlane *plane)
{
	if (!_planes)
		_planes = LArray::_alloc()->init();
	_planes->addObject(plane);
}

void CClipPlanesProvider::removeClipPlane(CClipPlane *plane)
{
	if (_planes)
		_planes->removeObject(plane);
}

void CClipPlanesProvider::removeAllClipPlanes()
{
	LRelease(_planes);
	_planes = 0;
}

bool CClipPlanesProvider::_getClipPlanes(CClipPlane ***planes, int *count)
{
	if (!_planes)
		return false;
	*planes = (CClipPlane **)_planes->objects();
	*count = _planes->count();
	return *count > 0;
}

bool CClipPlanesProvider::getClipPlanes(CClipPlane ***planes, int *count)
{
	*count = 0;
	for (CClipPlanesProvider *p = this; p; p = p->_nextClipPlanesProvider())
		if (p->_getClipPlanes(planes, count))
			break;
	return *count != 0;
}

CTextureProvider *CTextureProvider::initWithSource(CTextureProviderCallback *source)
{
	if (!source)
		LF_RAISE(NilPtrException());
	_source = source;
	_sourceOwner = dynamic_cast<LObject *>(source);
	if (!_sourceOwner)
		LF_RAISE(NilPtrException());
	_sourceOwner->retain();
	return this;
}

void CTextureProvider::dealloc()
{
	LRelease(_sourceOwner);
	_sourceOwner = 0;
	LObject::dealloc();
}

// ---------------------------------------------------------------------------
// CShaderStage.

CShaderStage::CShaderStage()
    : CContextObject(1), _shader(0), _valid(false), _index(0), _drawStageIndex(0), _condition(0),
      _conditionParser(0), _usesVS(false), _usesPS(false), _textureSources(0), _vsCode(0), _psCode(0), _env(0),
      _clipInClipSpace(false), _clipFixedFunction(false), _activateBlock(0), _deactivateBlock(0), _vsConsts(0),
      _vsBase(0), _vsSize(0), _psConsts(0), _psBase(0), _psSize(0), _envConstsValid(false), _objectID(0)
{
}

void CShaderStage::dealloc()
{
	LRelease(_condition);
	_condition = 0;
	LRelease(_conditionParser);
	_conditionParser = 0;
	LRelease(_activateBlock);
	_activateBlock = 0;
	LRelease(_deactivateBlock);
	_deactivateBlock = 0;
	LRelease(_vsConsts);
	_vsConsts = 0;
	LRelease(_psConsts);
	_psConsts = 0;
	LRelease(_vsCode);
	_vsCode = 0;
	LRelease(_psCode);
	_psCode = 0;
	delete _textureSources;
	_textureSources = 0;
	CContextObject::dealloc();
}

CRenderProfile *CShaderStage::renderProfile()
{
	return _shader->_container->_profile;
}

const SShaderConstDescr *CShaderStage::instanceVSConstantDescription()
{
	return _vsInstance.count() ? _vsInstance.items() : 0;
}

void CShaderStage::setVSCode(const LString *code)
{
	LRelease(_vsCode);
	_vsCode = LString::_alloc();
	_vsCode->setValue(code);
}

void CShaderStage::setPSCode(const LString *code)
{
	LRelease(_psCode);
	_psCode = LString::_alloc();
	_psCode->setValue(code);
}

// The constants are sorted by update class; the stage keeps a buffer for
// the registers from the lowest to the end of the highest.
void CShaderStage::setVSConstants(const SShaderConstDescr *items, long count)
{
	_vsStatic.removeAllObjects();
	_vsEnv.removeAllObjects();
	_vsDynamic.removeAllObjects();
	_vsInstance.removeAllObjects();
	if (count <= 0)
		return;
	_vsBase = 0x7fff;
	int maxReg = 0, maxIndex = 0;
	for (long i = 0; i < count; i++) {
		const SShaderConstDescr &d = items[i];
		if (maxReg < d.reg) {
			maxReg = d.reg;
			maxIndex = i;
		}
		if (d.reg < _vsBase)
			_vsBase = d.reg;
		switch (d.cls) {
		case kShaderConstEnvDepended:
			_vsEnv.addObject(d);
			break;
		case kShaderConstInstance:
			_vsInstance.addObject(d);
			break;
		case kShaderConstStatic:
			_vsStatic.addObject(d);
			break;
		default:
			_vsDynamic.addObject(d);
			break;
		}
	}
	_vsSize = maxReg - _vsBase + items[maxIndex].count;
	LRelease(_vsConsts);
	_vsConsts = LData::_alloc()->init();
	_vsConsts->setLength(_vsSize << 4);
}

void CShaderStage::setPSConstants(const SShaderConstDescr *items, long count)
{
	_psStatic.removeAllObjects();
	_psEnv.removeAllObjects();
	_psDynamic.removeAllObjects();
	if (count <= 0)
		return;
	_psBase = 0x7fff;
	int maxReg = 0, maxIndex = 0;
	for (long i = 0; i < count; i++) {
		const SShaderConstDescr &d = items[i];
		if (maxReg < d.reg) {
			maxReg = d.reg;
			maxIndex = i;
		}
		if (d.reg < _psBase)
			_psBase = d.reg;
		if (d.cls == kShaderConstStatic)
			_psStatic.addObject(d);
		else if (d.cls == kShaderConstEnvDepended)
			_psEnv.addObject(d);
		else
			_psDynamic.addObject(d);
	}
	_psSize = maxReg - _psBase + items[maxIndex].count;
	LRelease(_psConsts);
	_psConsts = LData::_alloc()->init();
	_psConsts->setLength(_psSize << 4);
}

// Items are [register, source name] or [register, x, y, z, w] (a literal).
void CShaderStage::addItemsFromArrayToArray(LArray *items, TArray<SShaderConstDescr> *out)
{
	int n = items->count();
	for (int i = 0; i < n; i++) {
		LArray *item = (LArray *)items->objectAtIndex(i);
		if (!item)
			continue;
		if (item->count() == 2) {
			LString *source = (LString *)item->objectAtIndex(1);
			SShaderConstInfo info = shaderConstParamsForKey(source);
			if (info.id == -1)
				continue;	// an unknown source
			SShaderConstDescr d;
			memset(&d, 0, sizeof d);
			d.reg = ((LString *)item->objectAtIndex(0))->intValue();
			d.cls = info.cls;
			d.count = info.count;
			d.ds = info.ds;
			d.id = info.id;
			out->addObject(d);
		} else if (item->count() == 5) {
			SShaderConstDescr d;
			d.reg = ((LString *)item->objectAtIndex(0))->intValue();
			d.cls = kShaderConstStatic;
			d.count = 1;
			d.ds = 0;
			d.id = kLiteralConstID;
			for (int k = 0; k < 4; k++)
				d.value[k] = ((LString *)item->objectAtIndex(k + 1))->floatValue();
			out->addObject(d);
		}
	}
}

void CShaderStage::processInitializationPair(const LObject *value, const LComparable *key, const LMap *)
{
	if (key->isEqualTo(&kShaderActivateBlock)) {
		if (!_activateBlock)
			_activateBlock = CStateBlock::_alloc()->initWithHolder(_holder);
		_activateBlock->setStaticStatesArray((LArray *)value);
	} else if (key->isEqualTo(&kShaderDeactivateBlock)) {
		_deactivateBlock = CStateBlock::_alloc()->initWithArray((LArray *)value, _holder);
	} else if (key->isEqualTo(&kShaderStageActivateCondition)) {
		_condition = LString::_alloc();
		_condition->setValue((const LString *)value);
	} else if (key->isEqualTo(&kObjectIDKey)) {
		_objectID = UMapParsingUtils::loadIntFromObject(value);
	} else if (key->isEqualTo(&kShaderStageTextureSources)) {
		delete _textureSources;
		_textureSources = 0;
		const LArray *sources = lobj_cast<const LArray>(value);
		if (!sources)
			LF_RAISE(NilPtrException());
		_textureSources = new TArray<int>;
		int n = sources->count();
		for (int i = 0; i < n; i++)
			_textureSources->addObject(textureSourceIDForKey(lobj_cast<LString>(sources->objectAtIndex(i))));
	} else if (key->isEqualTo(&kShaderStageVertexShader)) {
		_usesVS = ((const LString *)value)->length() > 4;
		setVSCode((const LString *)value);
		if (!_activateBlock)
			_activateBlock = CStateBlock::_alloc()->initWithHolder(_holder);
	} else if (key->isEqualTo(&kShaderStagePixelShader)) {
		_usesPS = ((const LString *)value)->length() > 4;
		setPSCode((const LString *)value);
		if (!_activateBlock)
			_activateBlock = CStateBlock::_alloc()->initWithHolder(_holder);
	} else if (key->isEqualTo(&kShaderStageVSConstants)) {
		TArray<SShaderConstDescr> items;
		addItemsFromArrayToArray((LArray *)value, &items);
		if (items.count() > 0)
			setVSConstants(items.items(), items.count());
	} else if (key->isEqualTo(&kShaderStagePSConstants)) {
		TArray<SShaderConstDescr> items;
		addItemsFromArrayToArray((LArray *)value, &items);
		if (items.count() > 0)
			setPSConstants(items.items(), items.count());
	}
	// Other keys are ignored.
}

unsigned int CShaderStage::digitsSeparatedByDotInStringAfterKey(const LString *text, const LString *key)
{
	unsigned int keyLength = key->length();
	if (keyLength == 0)
		return 0;
	LString *lowerKey = key->lowercaseString();
	LCharSet *digits = LCharSet::charSetWithType(LCharSet::kDecimalDigit);
	LCharSet *dots = LCharSet::characterSetWithCharactersInCString("._");
	LArray *lines = text->componentsSeparatedByLineBreaks(LCharSet::charSetWithType(LCharSet::kWhitespaceAndNewline));
	int n = lines->count();
	// The first non-empty line holds the key.
	for (int i = 0; i < n; i++) {
		LString *line = (LString *)lines->objectAtIndex(i);
		unsigned int length = line->length();
		if (length == 0)
			continue;
		if (length < keyLength)
			return 0;
		line = line->lowercaseString();
		unsigned int at = line->findString(lowerKey, 0, 1);
		if (at == ~0u)
			return 0;
		unsigned int major = line->findFirstCharFromSet(digits, at + keyLength, 1);
		if (major == ~0u)
			return 0;
		unsigned int dot = line->findFirstCharFromSet(dots, major, 1);
		if (dot == ~0u)
			return 0;
		unsigned int minor = line->findFirstCharFromSet(digits, dot, 1);
		if (minor == ~0u)
			return 0;
		unsigned int hi = line->substringWithRange(major, dot - major)->intValue();
		unsigned int lo = line->substringFromIndex(minor)->intValue();
		return (hi & 0xff) << 8 | (lo & 0xff);
	}
	return 0;
}

bool CShaderStage::validateInContext()
{
	LAutoreleasePool::addNewPoolToTop();
	_valid = false;
	CContextCaps *caps = _context->caps();
	bool ok = true;
	if (_textureSources && (unsigned int)_textureSources->count() > caps->GetMaxSimultTextures())
		ok = false;
	if (ok && _usesVS) {
		unsigned int version = digitsSeparatedByDotInStringAfterKey(_vsCode, &_sVSKey);
		ok = version > 0xff && (int)version <= caps->GetVertexShadersVer();
	}
	if (ok && _usesPS) {
		unsigned int version = digitsSeparatedByDotInStringAfterKey(_psCode, &_sPSKey);
		ok = version > 0xff && (int)version <= caps->GetPixelShadersVer();
	}
	LAutoreleasePool::removeTopPool();
	return ok;
}

bool CShaderStage::checkConditionForEnv(const CRenderEnvironment *)
{
	if (!_condition)
		return true;
	if (!_conditionParser) {
		_conditionParser = LExpressionParser::_alloc()->init();
		_conditionParser->precompileExpression(_condition, this);
	}
	return _conditionParser->evaluatePrecompiled() != 0;
}

const int *CShaderStage::addressOfVariable(LString *name)
{
	CRenderEnvironment *env = _context->renderEnvironment();
	if (name->isEqualTo(&kEnvParamClipState))
		return &env->_clipPlanesCount;
	if (name->isEqualTo(&kEnvParamSMCount))
		return &env->_shadowMapsCount;
	if (name->isEqualTo(&kEnvParamSMUpdateMode))
		return &env->_shadowMapUpdateMode;
	return 0;
}

bool CShaderStage::adjustToNewEnvironment(const CRenderEnvironment *env)
{
	_envConstsValid = false;
	_env = env;
	adjustEnvDependedVariables(env);
	if (!checkConditionForEnv(env))
		return false;
	_clipInClipSpace = false;
	_clipFixedFunction = false;
	if (env->_clipPlanesCount != 0 && !_shader->_notClippable && _shader->_clipPlanesSupported) {
		if (_usesVS)
			_clipInClipSpace = true;
		else
			_clipFixedFunction = true;
	}
	return true;
}

void CShaderStage::instantiateSelf()
{
	_valid = validateInContext();
	if (!_valid || !_activateBlock)
		return;
	DeviceEmu *device = _context->device();
	CDrawCache *drawCache = _context->drawCache();
	StStateRecordZone zone(_activateBlock);
	bool keepBlock = _activateBlock->_itemsCount != 0;
	if (_vsCode && _vsCode->length() >= 2) {
		CVertexShader vs(device, drawCache);
		vs.setShaderCode(_vsCode);
		vs.activateImmediately(device);
		keepBlock = true;
	} else {
		device->SetVertexShader(0);
	}
	if (_psCode && _psCode->length() >= 2) {
		CPixelShader ps(device, drawCache);
		ps.setShaderCode(_psCode);
		ps.activateImmediately(device);
		keepBlock = true;
	} else {
		device->SetPixelShader(0);
	}
	Matrix buffer;
	if (_vsConsts) {
		char *bytes = (char *)_vsConsts->bytes();
		for (int i = 0; i < _vsStatic.count(); i++) {
			const SShaderConstDescr &d = _vsStatic[i];
			const float *data = (const float *)dataPtrForConstant(0, &d, &buffer);
			if (!data)
				continue;
			memcpy(bytes + (d.reg - _vsBase) * 16, data, d.count * 16);
			device->SetVertexShaderConstantF(d.reg, data, d.count);
		}
	}
	if (_psConsts) {
		char *bytes = (char *)_psConsts->bytes();
		for (int i = 0; i < _psStatic.count(); i++) {
			const SShaderConstDescr &d = _psStatic[i];
			const float *data = (const float *)dataPtrForConstant(0, &d, &buffer);
			if (!data)
				continue;
			memcpy(bytes + (d.reg - _psBase) * 16, data, d.count * 16);
			device->SetPixelShaderConstantF(d.reg, data, d.count);
		}
	}
	if (!keepBlock) {
		LRelease(_activateBlock);
		_activateBlock = 0;
	}
}

void CShaderStage::releaseSelf()
{
	retain();
	LRelease(_conditionParser);
	_conditionParser = 0;
	_valid = false;
	_envConstsValid = false;
	release();
}

// Stages with programs drop and rebuild them around a context reset.
void CShaderStage::purgeSelf()
{
	if (!_managesSelf && ((!_vsCode && !_psCode) || !_activateBlock))
		return;
	releaseSelf();
}

void CShaderStage::restoreSelf()
{
	if (!_managesSelf && ((!_vsCode && !_psCode) || !_activateBlock))
		return;
	instantiateSelf();
}

int CShaderStage::offsetForConstant(int id)
{
	intptr_t cls = -1;
	sShaderConstIDToConstClassMap->getValueForKey(&cls, id);
	const CShaderConstsArray *a;
	switch (cls) {
	case kShaderConstEnvDepended:
		a = &_vsEnv;
		break;
	case kShaderConstInstance:
		a = &_vsInstance;
		break;
	case kShaderConstStatic:
		a = &_vsStatic;
		break;
	default:
		a = &_vsDynamic;
		break;
	}
	for (int i = 0; i < a->count(); i++)
		if ((*a)[i].id == id)
			return (*a)[i].reg;
	return -1;
}

CShadowEnvMap *CShaderStage::shadowMapFromProfileAtIndex(long index)
{
	return _context ? _context->renderEnvironment()->shadowMapAtIndex(index) : 0;
}

CConstDataProvider *CShaderStage::_nextConstDataProvider()
{
	CDrawableModelData *data = _shader ? _shader->currentModelData() : 0;
	return data ? FHModelDataConstProvider(data) : 0;
}

CInstanceConstDataProvider *CShaderStage::_nextInstanceConstDataProvider()
{
	CDrawableModelData *data = _shader ? _shader->currentModelData() : 0;
	return data ? FHModelDataInstanceConstProvider(data) : 0;
}

// A light's direction in object space, normalized and with w = 1.
static void _lightDirectionInObjectSpace(CContext *context, const D3DLIGHT9 *light, Vector4 *out)
{
	_SVector4 dir = {light->Direction.x, light->Direction.y, light->Direction.z, 0};
	FHVec4Transform(out, &dir, context->invWorldMatrix());
	Vector3 scaling;
	UMathUtils::scalingFromMatrix(context->worldMatrix(), &scaling);
	out->x *= scaling.x;
	out->y *= scaling.y;
	out->z *= scaling.z;
	FHVec3Normalize((_SVector3 *)out, (_SVector3 *)out);
	out->w = 1;
}

const void *CShaderStage::_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer)
{
	// pi, the sine and cosine series coefficients in units of 2 pi
	static const float pi[4] = {3.14159274f, 6.28318548f, 1.57079637f, 0.785398185f};
	static const float sinCoeffs[4] = {6.28318501f, -41.3417015f, 81.6052399f, -76.7058487f};
	static const float cosCoeffs[4] = {19.7392082f, -64.9393921f, 85.4568100f, -60.2446327f};
	CContext *context = _context;
	float *out = (float *)buffer;
	int id = descr->id;
	switch (id) {
	case 0:
		return pi;
	case 1:
		return sinCoeffs;
	case 2:
		return cosCoeffs;
	case kLiteralConstID:
		memcpy(out, descr->value, 16);
		return out;
	}
	if (!context)
		return 0;
	CRenderEnvironment *env = context->renderEnvironment();
	switch (id) {
	case 3:
		return context->worldViewProjMatrixTransp();
	case 4:
		return context->invWorldMatrixTransp();
	case 5:
		return context->worldMatrixTransp();
	case 6:
		return context->viewProjMatrixTransp();
	case 7:
		return context->viewMatrixTransp();
	case 8: {
		LSize size = context->holder()->relativeFrameSize();
		out[0] = size.width;
		out[1] = size.height;
		out[2] = 1.0f / size.width;
		out[3] = 1.0f / size.height;
		return out;
	}
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
		// (The original handed out the CClipPlane object itself here.)
		return env->clipPlaneAtIndex(id - 10)->plane();
	case 16:
	case 17:
	case 18:
	case 19:
	case 20:
	case 21:
		FHPlaneTransform((_SPlane *)out, env->clipPlaneAtIndex(id - 16)->plane(), context->worldMatrixTransp());
		return out;
	case 22: {
		const Vector3 *eye = context->mainCamera()->position();
		out[0] = eye->x;
		out[1] = eye->y;
		out[2] = eye->z;
		out[3] = 1;
		return out;
	}
	case 23: {
		const Vector3 *eye = context->mainCamera()->position();
		_SVector4 v = {eye->x, eye->y, eye->z, 1};
		return FHVec4Transform((_SVector4 *)out, &v, context->invWorldMatrix());
	}
	case 24:
		return &env->_ambient;
	case 25:
	case 29:
	case 33:
	case 36:
		return &env->environmentLightAtIndex(id == 25 ? 0 : id == 29 ? 1 : id == 33 ? 2 : 3)->Direction;
	case 26:
	case 30:
	case 34:
	case 37:
		_lightDirectionInObjectSpace(context,
		                             env->environmentLightAtIndex(id == 26 ? 0 : id == 30 ? 1 : id == 34 ? 2 : 3),
		                             (Vector4 *)out);
		return out;
	case 27:
	case 31:
	case 35:
	case 38:
		return &env->environmentLightAtIndex(id == 27 ? 0 : id == 31 ? 1 : id == 35 ? 2 : 3)->Diffuse;
	case 63:
	case 64: {
		CShadowEnvMap *map = shadowMapFromProfileAtIndex(id - 63);
		if (map) {
			FHMatrixMultiply(buffer, context->worldMatrix(), map->textureMatrix());
			return FHMatrixTranspose(buffer, buffer);
		}
		memset(buffer, 0, sizeof *buffer);
		return buffer;
	}
	case 65: {
		Color fog = context->currentRenderTarget()->fogColor();
		memcpy(out, &fog, 16);
		return out;
	}
	case 66:
		UMathUtils::fillExp2FogParamsConstant(out, context->worldViewMatrixTransp(),
		                                      context->mainCamera()->farClip() * 0.5f,
		                                      context->mainCamera()->farClip(), 2.3f);
		return out;
	}
	return 0;
}

void CShaderStage::_updateEnvDependedConstantsIfNeeded(int *vsFirst, int *vsEnd, int *psFirst, int *psEnd)
{
	if (_envConstsValid)
		return;
	_envConstsValid = true;
	Matrix buffer;
	if (_vsConsts) {
		char *bytes = (char *)_vsConsts->bytes();
		for (int i = 0; i < _vsEnv.count(); i++) {
			const SShaderConstDescr &d = _vsEnv[i];
			const void *data = dataPtrForConstant(0, &d, &buffer);
			if (data)
				memcpy(bytes + (d.reg - _vsBase) * 16, data, d.count * 16);
			if (d.reg < *vsFirst)
				*vsFirst = d.reg;
			if (*vsEnd < d.reg + d.count)
				*vsEnd = d.reg + d.count;
		}
	}
	if (_psConsts) {
		char *bytes = (char *)_psConsts->bytes();
		for (int i = 0; i < _psEnv.count(); i++) {
			const SShaderConstDescr &d = _psEnv[i];
			const void *data = dataPtrForConstant(0, &d, &buffer);
			if (data)
				memcpy(bytes + (d.reg - _psBase) * 16, data, d.count * 16);
			if (d.reg < *psFirst)
				*psFirst = d.reg;
			if (*psEnd < d.reg + d.count)
				*psEnd = d.reg + d.count;
		}
	}
}

// Fills the dynamic constants and passes the changed register range on;
// on a first activation the environment's constants are refreshed too.
void CShaderStage::_passConstantsToDevice(bool reactivation)
{
	CDrawCache *drawCache = _context->drawCache();
	int vsFirst = 0x7fff, vsEnd = 0, psFirst = 0x7fff, psEnd = 0;
	if (!reactivation)
		_updateEnvDependedConstantsIfNeeded(&vsFirst, &vsEnd, &psFirst, &psEnd);
	CDrawableModelData *data = _shader->currentModelData();
	Matrix buffer;
	if (_vsConsts) {
		char *base = (char *)_vsConsts->bytes() - _vsBase * 16;
		for (int i = 0; i < _vsDynamic.count(); i++) {
			const SShaderConstDescr &d = _vsDynamic[i];
			const void *ptr = dataPtrForConstant(data, &d, &buffer);
			if (!ptr)
				continue;
			memcpy(base + d.reg * 16, ptr, d.count * 16);
			if (d.reg < vsFirst)
				vsFirst = d.reg;
			if (vsEnd < d.reg + d.count)
				vsEnd = d.reg + d.count;
		}
		if (vsEnd - vsFirst > 0)
			drawCache->setVSConstant(vsFirst, (const float *)(base + vsFirst * 16), vsEnd - vsFirst);
	}
	if (_psConsts) {
		char *base = (char *)_psConsts->bytes() - _psBase * 16;
		for (int i = 0; i < _psDynamic.count(); i++) {
			const SShaderConstDescr &d = _psDynamic[i];
			const void *ptr = dataPtrForConstant(data, &d, &buffer);
			if (!ptr)
				continue;
			memcpy(base + d.reg * 16, ptr, d.count * 16);
			if (d.reg < psFirst)
				psFirst = d.reg;
			if (psEnd < d.reg + d.count)
				psEnd = d.reg + d.count;
		}
		if (psEnd - psFirst > 0)
			drawCache->setPSConstant(psFirst, (const float *)(base + psFirst * 16), psEnd - psFirst);
	}
}

void CShaderStage::_resetClipSettings()
{
	int n = _env->clipPlanesCount();
	if (n > 6)
		n = 6;
	CDrawCache *drawCache = _context->drawCache();
	if (_clipInClipSpace) {
		const Matrix *m = _context->invViewProjMatrixTransp();
		for (int i = 0; i < n; i++) {
			Plane plane;
			FHPlaneTransform(&plane, _env->clipPlaneAtIndex(i)->plane(), m);
			drawCache->setClipPlane(i, &plane);
		}
	} else if (_clipFixedFunction) {
		for (int i = 0; i < n; i++)
			drawCache->setClipPlane(i, _env->clipPlaneAtIndex(i)->plane());
	}
}

// A model may switch stages of shaders with dynamic stages off.
static bool _stageIsOff(CShaderStage *stage)
{
	CShader *shader = stage->shader();
	CDrawableModelData *data = shader->currentModelData();
	return shader->_dynamicStages && data && !FHModelDataStageStateProvider(data)->isStageOn(stage);
}

bool CShaderStage::activateShaderStage()
{
	if (_stageIsOff(this))
		return false;
	CDrawCache *drawCache = _context->drawCache();
	if (!_usesVS) {
		_context->passWorldMatrixToDevIfNeeded();
		drawCache->setVertexShader(0);
	}
	_passConstantsToDevice(false);
	if (_activateBlock)
		drawCache->applyStateBlock(_activateBlock);
	if (_clipInClipSpace || _clipFixedFunction)
		_resetClipSettings();
	return true;
}

bool CShaderStage::reactivateShaderStage()
{
	if (_stageIsOff(this))
		return false;
	if (!_usesVS)
		_context->passWorldMatrixToDevIfNeeded();
	_passConstantsToDevice(true);
	if (_clipInClipSpace)
		_resetClipSettings();
	return true;
}

void CShaderStage::deactivateShaderStage()
{
	CDrawCache *drawCache = _context->drawCache();
	if (_deactivateBlock)
		drawCache->applyStateBlock(_deactivateBlock);
	if (_clipInClipSpace || _clipFixedFunction)
		drawCache->setRenderState(D3DRS_CLIPPLANEENABLE, 0);
}
