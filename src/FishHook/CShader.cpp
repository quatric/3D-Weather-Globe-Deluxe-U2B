// CShader: a render profile's shader, its vertex declaration and stages.

#include "FHRender.h"
#include "LScanner.h"

LF_IMPLEMENT_CLASS(CShader, LF_CLASSOF(CContextObject))

LConstantString kShaderRequirements("Requirements");
LConstantString kShaderActivateBlock("ActivateBlock");
LConstantString kShaderDeactivateBlock("DeactivateBlock");
LConstantString kShaderIsTransparent("IsTransparent");
LConstantString kShaderPSClipPlanesCount("PSClipPlanesCount");
LConstantString kShaderNotClippable("IsNotClippable");
LConstantString kShaderForceSWVP("ForceSWVP");
LConstantString kShaderMaxLightsCount("MaxLightsCount");
LConstantString kShaderCanDropShadow("CanDropShadow");
LConstantString kShaderDynamicStages("DynamicStages");
LConstantString kShaderMaxBlendIndex("MaxBlendIndex");
LConstantString kShaderStages("Stages");
LConstantString kShaderStreams("Streams");
LConstantString kShaderDynamicBuffer("DynamicBuffer");
LConstantString kShaderStreamIsInstancingStream("IsInstancing");
LConstantString kShaderStreamDecl("StreamDecl");
LConstantString kShaderStreamUsageKey("Usage");
LConstantString kShaderStreamTypeKey("Format");
LConstantString kDevicePropClipPlanesCount("ClipPlanesCount");
LConstantString kDevicePropVSVersion("VSVersion");
LConstantString kDevicePropPSVersion("PSVersion");
LConstantString kDevicePropTSCount("TextureStagesCount");
LConstantString kDevicePropVolTextures("VolumeTextures");
LConstantString kDevicePropDepthTextures("DepthTextures");

LMap *CShader::sUsageValuesMap = 0;
LMap *CShader::sUsageIndexesMap = 0;
LMap *CShader::sFormatValuesMap = 0;
LMap *CShader::sDefaultFormatValuesMap = 0;

void CShaderCleaner::performCleanup()
{
	LRelease(CShader::sUsageValuesMap);
	CShader::sUsageValuesMap = 0;
	LRelease(CShader::sUsageIndexesMap);
	CShader::sUsageIndexesMap = 0;
	LRelease(CShader::sFormatValuesMap);
	CShader::sFormatValuesMap = 0;
	LRelease(CShader::sDefaultFormatValuesMap);
	CShader::sDefaultFormatValuesMap = 0;
}
static CShaderCleaner _sShaderCleaner;

CShader::CShader()
    : CContextObject(2), _container(0), _valid(false), _stages(LArray::_alloc()->init()),
      _activeStages(new TArray<CShaderStage *>), _requirements(0), _requirementsParser(0), _vertexDecl(0),
      _activateBlock(0), _deactivateBlock(0), _isTransparent(false), _forceSWVP(false), _dynamicStages(false),
      _maxLightsCount(2), _canDropShadow(false), _psClipPlanesCount(0), _notClippable(false), _maxTextureStages(0),
      _maxBlendIndex(0), _streamsCount(0), _texcoordsCount(0), _clipPlanesCount(0), _useSWVP(true),
      _clipPlanesSupported(true), _currentStage(0), _modelData(0)
{
	memset(_dynamicBuffer, 0, sizeof _dynamicBuffer);
	memset(_instancingStream, 0, sizeof _instancingStream);
	memset(_decl, 0, sizeof _decl);
	static const D3DVERTEXELEMENT9 end = D3DDECL_END();
	_decl[0] = end;
}

void CShader::dealloc()
{
	LRelease(_currentStage);
	_currentStage = 0;
	if (_modelData)
		FHModelDataObject(_modelData)->release();
	_modelData = 0;
	LRelease(_requirements);
	_requirements = 0;
	LRelease(_stages);
	_stages = 0;
	LRelease(_activateBlock);
	_activateBlock = 0;
	LRelease(_deactivateBlock);
	_deactivateBlock = 0;
	delete _activeStages;
	_activeStages = 0;
	CContextObject::dealloc();
}

// Keeps the model data the shader draws for.
static void _setModelData(CDrawableModelData *&slot, CDrawableModelData *data)
{
	if (data)
		FHModelDataObject(data)->retain();
	if (slot)
		FHModelDataObject(slot)->release();
	slot = data;
}

CShader *CShader::initWithMapAndContext(const LMap *map, void *context)
{
	CContextObject::initWithMapAndContext(map, context);
	if (!sUsageValuesMap) {
		sUsageValuesMap = LMap::_alloc()->init();
		sUsageIndexesMap = LMap::_alloc()->init();
		sFormatValuesMap = LMap::_alloc()->init();
		sDefaultFormatValuesMap = LMap::_alloc()->init();
		InsertIntValueForKey(sUsageValuesMap, D3DDECLUSAGE_POSITION, "position");
		InsertIntValueForKey(sUsageValuesMap, D3DDECLUSAGE_POSITIONT, "positiontransformed");
		InsertIntValueForKey(sUsageValuesMap, D3DDECLUSAGE_NORMAL, "normal");
		InsertIntValueForKey(sUsageValuesMap, D3DDECLUSAGE_TEXCOORD, "texcoord");
		InsertIntValueForKey(sUsageValuesMap, D3DDECLUSAGE_COLOR, "diffuse");
		InsertIntValueForKey(sUsageValuesMap, D3DDECLUSAGE_COLOR, "specular");
		InsertIntValueForKey(sUsageValuesMap, D3DDECLUSAGE_PSIZE, "pointsize");
		InsertIntValueForKey(sUsageValuesMap, D3DDECLUSAGE_FOG, "fog");
		InsertIntValueForKey(sUsageValuesMap, D3DDECLUSAGE_TANGENT, "tangent");
		InsertIntValueForKey(sUsageValuesMap, D3DDECLUSAGE_BINORMAL, "binormal");
		InsertIntValueForKey(sUsageIndexesMap, 1, "specular");
		InsertIntValueForKey(sDefaultFormatValuesMap, D3DDECLTYPE_FLOAT3, "position");
		InsertIntValueForKey(sDefaultFormatValuesMap, D3DDECLTYPE_FLOAT4, "positiontransformed");
		InsertIntValueForKey(sDefaultFormatValuesMap, D3DDECLTYPE_FLOAT3, "normal");
		InsertIntValueForKey(sDefaultFormatValuesMap, D3DDECLTYPE_FLOAT2, "texcoord");
		InsertIntValueForKey(sDefaultFormatValuesMap, D3DDECLTYPE_D3DCOLOR, "diffuse");
		InsertIntValueForKey(sDefaultFormatValuesMap, D3DDECLTYPE_D3DCOLOR, "specular");
		InsertIntValueForKey(sDefaultFormatValuesMap, D3DDECLTYPE_FLOAT1, "pointsize");
		InsertIntValueForKey(sDefaultFormatValuesMap, D3DDECLTYPE_FLOAT1, "fog");
		InsertIntValueForKey(sDefaultFormatValuesMap, D3DDECLTYPE_FLOAT3, "tangent");
		InsertIntValueForKey(sDefaultFormatValuesMap, D3DDECLTYPE_FLOAT3, "binormal");
		InsertIntValueForKey(sFormatValuesMap, D3DDECLTYPE_FLOAT1, "float");
		InsertIntValueForKey(sFormatValuesMap, D3DDECLTYPE_FLOAT1, "float1");
		InsertIntValueForKey(sFormatValuesMap, D3DDECLTYPE_FLOAT2, "float2");
		InsertIntValueForKey(sFormatValuesMap, D3DDECLTYPE_FLOAT3, "float3");
		InsertIntValueForKey(sFormatValuesMap, D3DDECLTYPE_FLOAT4, "float4");
		InsertIntValueForKey(sFormatValuesMap, D3DDECLTYPE_D3DCOLOR, "d3dcolor");
		InsertIntValueForKey(sFormatValuesMap, D3DDECLTYPE_D3DCOLOR, "ubyte4");
	}
	return this;
}

void CShader::processInitializationPair(const LObject *value, const LComparable *key, const LMap *)
{
	const LString *string = (const LString *)value;
	if (key->isEqualTo(&kShaderRequirements)) {
		_requirements = LString::_alloc();
		_requirements->setValue(string);
	} else if (key->isEqualTo(&kShaderMaxLightsCount)) {
		_maxLightsCount = string->intValue();
	} else if (key->isEqualTo(&kShaderIsTransparent)) {
		_isTransparent = string->boolValue();
	} else if (key->isEqualTo(&kShaderPSClipPlanesCount)) {
		_psClipPlanesCount = UMapParsingUtils::loadIntFromObject(value);
	} else if (key->isEqualTo(&kShaderNotClippable)) {
		_notClippable = string->boolValue();
	} else if (key->isEqualTo(&kShaderForceSWVP)) {
		_forceSWVP = string->boolValue();
	} else if (key->isEqualTo(&kShaderCanDropShadow)) {
		_canDropShadow = string->boolValue();
	} else if (key->isEqualTo(&kShaderDynamicStages)) {
		_dynamicStages = string->boolValue();
	} else if (key->isEqualTo(&kShaderMaxBlendIndex)) {
		_maxBlendIndex = string->intValue();
	} else if (key->isEqualTo(&kShaderActivateBlock)) {
		_activateBlock = CStateBlock::_alloc()->initWithArray((LArray *)value, _holder);
	} else if (key->isEqualTo(&kShaderDeactivateBlock)) {
		_deactivateBlock = CStateBlock::_alloc()->initWithArray((LArray *)value, _holder);
	} else if (key->isEqualTo(&kShaderStreams)) {
		const LArray *streams = lobj_cast<const LArray>(value);
		if (!streams)
			LF_RAISE(NilPtrException());
		_buildVertexDeclWithArray(streams);
	} else if (key->isEqualTo(&kShaderStages)) {
		const LArray *stages = lobj_cast<const LArray>(value);
		if (!stages)
			LF_RAISE(NilPtrException());
		int n = stages->count();
		for (int i = 0; i < n; i++) {
			LMap *map = lobj_cast<LMap>(stages->objectAtIndex(i));
			if (!map)
				LF_RAISE(NilPtrException());
			CShaderStage *stage = lobj_cast<CShaderStage>(CContextObject::contextObjectWithMap(map, _holder));
			stage->_shader = this;
			addShaderStage(stage);
		}
	}
	// Other keys are ignored.
}

// Each stream is a map with its elements under "StreamDecl"; an element has
// a usage and an optional format (the usage's default otherwise).
void CShader::_buildVertexDeclWithArray(const LArray *streams)
{
	_texcoordsCount = 0;
	_streamsCount = streams->count();
	int element = 0;
	for (int s = 0; s < _streamsCount; s++) {
		LMap *map = lobj_cast<LMap>(streams->objectAtIndex(s));
		if (s < kMaxShaderStreams) {
			_dynamicBuffer[s] = UMapParsingUtils::loadIntFromObject(map->objectForKey(&kShaderDynamicBuffer)) != 0;
			_instancingStream[s] =
			    UMapParsingUtils::loadIntFromObject(map->objectForKey(&kShaderStreamIsInstancingStream)) != 0;
		}
		LArray *decl = map ? lobj_cast<LArray>(map->objectForKey(&kShaderStreamDecl)) : 0;
		if (!decl)
			LF_RAISE(NilPtrException());
		int n = decl->count();
		unsigned short offset = 0;
		for (int i = 0; i < n && element < kMaxDeclElements - 1; i++, element++) {
			LMap *item = lobj_cast<LMap>(decl->objectAtIndex(i));
			if (!item)
				LF_RAISE(NilPtrException());
			LString *usageName = (LString *)item->objectForKey(&kShaderStreamUsageKey);
			if (!usageName)
				LF_RAISE(FailedWithCodeException(-50));
			usageName = usageName->lowercaseString();
			LValue *usage = (LValue *)sUsageValuesMap->objectForKey(usageName);
			if (!usage)
				LF_RAISE(FailedWithCodeException(-50));
			D3DVERTEXELEMENT9 &e = _decl[element];
			e.Stream = s;
			e.Offset = offset;
			e.Method = 0;
			e.Usage = usage->intValue();
			if (e.Usage == D3DDECLUSAGE_TEXCOORD) {
				e.UsageIndex = _texcoordsCount++;
			} else {
				LValue *index = (LValue *)sUsageIndexesMap->objectForKey(usageName);
				e.UsageIndex = index ? index->intValue() : 0;
			}
			LString *formatName = (LString *)item->objectForKey(&kShaderStreamTypeKey);
			LValue *format = formatName ? (LValue *)sFormatValuesMap->objectForKey(formatName->lowercaseString())
			                            : (LValue *)sDefaultFormatValuesMap->objectForKey(usageName);
			if (!format)
				LF_RAISE(FailedWithCodeException(-50));
			e.Type = format->intValue();
			switch (e.Type) {
			case D3DDECLTYPE_FLOAT1:
			case D3DDECLTYPE_D3DCOLOR:
			case D3DDECLTYPE_UBYTE4:
				offset += 4;
				break;
			case D3DDECLTYPE_FLOAT2:
				offset += 8;
				break;
			case D3DDECLTYPE_FLOAT3:
				offset += 12;
				break;
			case D3DDECLTYPE_FLOAT4:
				offset += 16;
				break;
			}
		}
	}
	static const D3DVERTEXELEMENT9 end = D3DDECL_END();
	_decl[element] = end;
}

void CShader::addShaderStage(CShaderStage *stage)
{
	stage->_index = _stages->count();
	_stages->addObject(stage);
	int n = stage->textureSourcesCount();
	if (_maxTextureStages < n)
		_maxTextureStages = n;
}

bool CShader::satisfiesRequirements()
{
	if (!_requirements)
		return true;
	if (!_requirementsParser) {
		_requirementsParser = LExpressionParser::_alloc()->init();
		_requirementsParser->precompileExpression(_requirements, this);
	}
	return _requirementsParser->evaluatePrecompiled() != 0;
}

const int *CShader::addressOfVariable(LString *name)
{
	static const int none = 0;
	CContextCaps *caps = _context->caps();
	if (name->isEqualTo(&kDevicePropClipPlanesCount))
		return (const int *)&caps->_maxClipPlanes;
	if (name->isEqualTo(&kDevicePropVSVersion))
		return &caps->_vsVer;
	if (name->isEqualTo(&kDevicePropPSVersion))
		return &caps->_psVer;
	if (name->isEqualTo(&kDevicePropTSCount))
		return (const int *)&caps->_maxTUs;
	if (name->isEqualTo(&kDevicePropVolTextures) || name->isEqualTo(&kDevicePropDepthTextures))
		return &none;	// neither is supported
	return 0;
}

bool CShader::validateInContext()
{
	if (!satisfiesRequirements())
		return false;
	int n = _stages->count();
	if (n == 0)
		return false;
	for (int i = 0; i < n; i++)
		if (!((CShaderStage *)_stages->objectAtIndex(i))->isValid())
			return false;
	return true;
}

void CShader::instantiateSelf()
{
	_valid = validateInContext();
	if (!_valid)
		return;
	if (_streamsCount < 1)
		LF_RAISE(WrongConditionException());
	HRESULT hr = _context->device()->CreateVertexDeclaration(_decl, &_vertexDecl);
	if (hr < 0)
		LF_RAISE(FailedWithCodeException(hr));
	// Clip planes go to the device unless it has none or the shader can't be
	// clipped; software vertex processing is needed when only the fragment
	// program could clip.
	if (_context->caps()->GetMaxClipPlanesCount() == 0 || _notClippable) {
		if (_psClipPlanesCount < 1 && !_notClippable) {
			_clipPlanesSupported = true;
			_useSWVP = true;
		} else {
			_clipPlanesSupported = false;
			_useSWVP = false;
		}
	} else {
		_clipPlanesSupported = true;
		_useSWVP = false;
	}
	if (_forceSWVP)
		_useSWVP = true;
}

void CShader::releaseSelf()
{
	LRelease(_requirementsParser);
	_requirementsParser = 0;
	_setModelData(_modelData, 0);
	_valid = false;
	if (_activeStages)
		_activeStages->removeAllObjects();
	FHSafeRelease(_vertexDecl);
}

void CShader::adjustToNewEnvironment(const CRenderEnvironment *env)
{
	if (!_valid)
		return;
	_activeStages->removeAllObjects();
	// While shadow maps are drawn only shaders that cast shadows draw.
	if (env->_shadowMapUpdateMode != 0 && !_canDropShadow)
		return;
	_clipPlanesCount = env->clipPlanesCount();
	int n = _stages->count();
	for (int i = 0; i < n; i++) {
		CShaderStage *stage = (CShaderStage *)_stages->objectAtIndex(i);
		if (stage->adjustToNewEnvironment(env)) {
			stage->_drawStageIndex = _activeStages->count();
			_activeStages->addObject(stage);
		}
	}
}

void CShader::reloadShaderStages()
{
	adjustToNewEnvironment(_context->renderEnvironment());
}

long CShader::usedTextureStagesCount() const
{
	return _currentStage ? _currentStage->textureSourcesCount() : 0;
}

long CShader::activateShader(CDrawableModelData *data)
{
	CDrawCache *drawCache = _context->drawCache();
	_setModelData(_modelData, data);
	drawCache->setVertexDeclaration(_vertexDecl);
	if (_activateBlock)
		drawCache->applyStateBlock(_activateBlock);
	_context->setSoftwareVertexProcessing(_useSWVP);
	unsigned int planes = 0;
	if (_clipPlanesCount > 0 && _clipPlanesSupported && _clipPlanesCount <= 6)
		planes = (1u << _clipPlanesCount) - 1;
	drawCache->setRenderState(D3DRS_CLIPPLANEENABLE, planes);
	return _activeStages->count();
}

void CShader::reactivateShader(CDrawableModelData *data)
{
	_setModelData(_modelData, data);
}

bool CShader::activateShaderStage(long index)
{
	if (_currentStage) {
		_currentStage->deactivateShaderStage();
		LRelease(_currentStage);
		_currentStage = 0;
	}
	_currentStage = LRetain((*_activeStages)[index]);
	if (_currentStage->activateShaderStage())
		return true;
	LRelease(_currentStage);
	_currentStage = 0;
	return false;
}

void CShader::deactivateShader()
{
	if (_currentStage) {
		_currentStage->deactivateShaderStage();
		LRelease(_currentStage);
		_currentStage = 0;
	}
	_setModelData(_modelData, 0);
	CDrawCache *drawCache = _context->drawCache();
	if (_deactivateBlock)
		drawCache->applyStateBlock(_deactivateBlock);
	if (_useSWVP)
		_context->setSoftwareVertexProcessing(false);
	drawCache->setPixelShader(0);
	drawCache->setVertexShader(0);
}
