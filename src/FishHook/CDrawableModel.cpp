// CDrawableModel, CDrawableModelData and ModelDataWithMatrix.

#include "FHScene.h"

LF_IMPLEMENT_CLASS(CEnvironmentMap, LF_CLASSOF(CContextObject))
LF_IMPLEMENT_CLASS(CDrawableModel, LF_CLASSOF(CContextObject))
LF_IMPLEMENT_CLASS(CDrawableModelData, 0)
LF_IMPLEMENT_CLASS(ModelDataWithMatrix, LF_CLASSOF(CDrawableModelData))

// ---------------------------------------------------------------------------
// CDrawableModel

CDrawableModel::CDrawableModel(TCtxObjPriority priority) : CContextObject(priority), _currentData(0), _drawableType(0)
{
}

CDrawableModelData *CDrawableModel::detachModelDataInstance()
{
	LClass *cls = modelDataClass();
	LF_CHECK_PTR(cls);
	LObject *obj = LAutorelease(cls->alloc());
	CDrawableModelData *data = obj && obj->getClass()->isSubclassOf(LF_CLASSOF(CDrawableModelData))
	                               ? (CDrawableModelData *)obj
	                               : 0;
	LF_CHECK_PTR(data);
	return data->initWithHostModel(this);
}

int CDrawableModel::evaluateDrawableType()
{
	CRenderProfile *profile = renderProfile();
	return profile && profile->_isTransparent ? 2 : 1;
}

// ---------------------------------------------------------------------------
// CDrawableModelData

CDrawableModelData::CDrawableModelData() : _distance(0), _model(0), _profile(0)
{
	_center.x = _center.y = _center.z = 0;
}

CDrawableModelData *CDrawableModelData::initWithHostModel(CDrawableModel *model)
{
	_model = LRetain(model);
	_profile = _model->renderProfile();
	return this;
}

void CDrawableModelData::dealloc()
{
	LRelease(_model);
	_model = 0;
	LObject::dealloc();
}

const Matrix *CDrawableModelData::worldMatrix()
{
	return sIdentityMatrixPtr;
}

// Draws through the model with the data's clip planes added to the
// environment.
void CDrawableModelData::immediateDraw(CFrameDrawer *drawer)
{
	CDrawableModel *model = _model;
	CDrawableModelData *saved = model->_currentData;
	model->_currentData = this;
	if (!_model->hasRenderProfile())
		_model->_context->resetRenderProfiles();
	CClipPlane **planes = 0;
	int count = 0;
	CRenderEnvironment *env = drawer->_context->_renderEnvironment;
	CRenderProfile *profile = _model->renderProfile();
	if (getClipPlanes(&planes, &count))
		for (int i = 0; i < count; i++)
			env->addClipPlane(planes[i], profile);
	else
		count = 0;
	_model->doImmediateDrawWithData(this, drawer);
	for (int i = count - 1; i >= 0; i--)
		env->removeClipPlane(planes[i], profile);
	model->_currentData = saved;
}

void CDrawableModelData::renderSelfHolderInScene(CScene *scene, unsigned int order)
{
	scene->frameDrawer()->renderDrawableData(this, order);
}

// Sources 0xb and 0xc are the context's specular and Fresnel tables.
bool CDrawableModelData::_getTextureForSource(BaseTextureEmu **texture, int source)
{
	if (source == 0xb)
		*texture = (BaseTextureEmu *)_model->_context->specularTable();
	else if (source == 0xc)
		*texture = (BaseTextureEmu *)_model->_context->fresnelTable();
	else
		return CDeviceTextureProvider::_getTextureForSource(texture, source);
	return true;
}

bool CDrawableModelData::applyMaterialForSubset(long, CShaderStage *stage)
{
	CDrawCache *cache = _model->_context->_drawCache;
	cache->startTexturesSetup();
	if (stage) {
		int count = stage->textureSourcesCount();
		for (int i = 0; i < count; i++)
			cache->setTexture(i, textureForSource(stage->textureSourceAtIndex(i)));
	}
	cache->endTexturesSetup();
	return true;
}

void CDrawableModelData::releaseWithModel()
{
	_model->release();
	release();
}

void CDrawableModelData::retainWithModel()
{
	retain();
	_model->retain();
}

LObject *FHModelDataObject(CDrawableModelData *data)
{
	return data;
}

CConstDataProvider *FHModelDataConstProvider(CDrawableModelData *data)
{
	return data;
}

CInstanceConstDataProvider *FHModelDataInstanceConstProvider(CDrawableModelData *data)
{
	return data;
}

CShaderStageStateProvider *FHModelDataStageStateProvider(CDrawableModelData *data)
{
	return data;
}

// ---------------------------------------------------------------------------
// ModelDataWithMatrix

ModelDataWithMatrix::ModelDataWithMatrix()
{
	FHMatrixIdentity(&_matrix);
}

void ModelDataWithMatrix::storeSceneStateInStream(LData *stream, unsigned int)
{
	storeMatrixInStream(&_matrix, stream);
}

const void *ModelDataWithMatrix::restoreSceneStateFromData(const void *data, unsigned int)
{
	return loadMatrixFromStream(data, &_matrix);
}
