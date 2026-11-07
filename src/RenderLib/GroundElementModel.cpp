// A ground element's adaptive mesh: the water and city light constants, and
// the DEM loads its LODs need (from LOD 8; up to LOD 8 without the DEM).

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(GroundElementModel, LF_CLASSOF(GlobeAdaptiveMesh))
LF_SELECTOR(GroundElementModel, _reliefStateChanged);

static int sNumPosts;	// for profiling

GroundElementModel *GroundElementModel::initWithGroundElement(GroundElement *element, CRenderProfile *profile)
{
	_element = element;
	if (!CAdaptiveFlatMesh::initWithDataSource(element, profile, 0))
		return 0;
	LNotificationCenter::center()->addObserver(this, "_reliefStateChanged", Preferences::kRenderWith3DStateWasChanged,
	                                           0);
	return this;
}

void GroundElementModel::dealloc()
{
	LNotificationCenter::center()->removeObserver(this, Preferences::kRenderWith3DStateWasChanged, kLock);
	CAdaptiveFlatMesh::dealloc();
}

const void *GroundElementModel::_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer)
{
	switch (descr->id) {
	case 'AnWP': {
		// The polar rows fade the water out over 9 rows.
		int y = _element->_id.y;
		float from, to;
		if (y < 0x52) {
			from = (float)y - 9.0f;
			to = (float)y - 8.0f;
		} else {
			from = 153.0f - (float)y;
			to = 152.0f - (float)y;
		}
		float *v = (float *)buffer;
		v[3] = clamp01(to / 9.0f);
		v[2] = clamp01(from / 9.0f);
		v[1] = GroundElementsManager::mWaterTextureScale;
		v[0] = GroundElementsManager::mAnimatedWaterVisibility;
		return buffer;
	}
	case 'AnWQ':
		return &GroundElementsManager::mDarkWaterColor;
	case 'AnWR':
		return &GroundElementsManager::mLightWaterColor;
	case 'AnWS':
		return &GroundElementsManager::mLightDarkDiffColor;
	case 'AnWT':
		return &GroundElementsManager::mWaterSpecularColor;
	case 'TTrp':
		return &GroundElementsManager::mNightCityGlowsColor;
	}
	return CAdaptiveFlatMesh::_dataPtrForConstant(descr, buffer);
}

void GroundElementModel::drawSubset(CDrawableModelData *data, long subset, long index)
{
	unsigned int factor;
	if (index == 0) {
		float alpha = GroundElementsManager::mAnimatedWaterVisibility * 1.8f - 0.8f;
		if (alpha < 0.0f)
			alpha = 0.0f;
		else if (alpha > 0.999f)
			alpha = 0.999f;
		alpha *= 255.0f;
		if (alpha <= 0.0f)
			alpha = 0.0f;
		unsigned int a = (unsigned int)alpha;
		factor = a << 8 | a | a << 16 | a << 24;
	} else if (index == 1) {
		factor = (unsigned int)GroundElementsManager::mLightWaterColor;
	} else {
		CAdaptiveFlatMesh::drawSubset(data, subset, index);
		return;
	}
	_context->device()->SetRenderState(D3DRS_TEXTUREFACTOR, factor);
	CAdaptiveFlatMesh::drawSubset(data, subset, index);
}

bool GroundElementModel::_isStageOn(CShaderStage *stage, unsigned char *on)
{
	bool dayNight = *RenderOptions::dayNightIsEnabledFlagPtr() != 0;
	bool water = _element->_hasWater && GroundElementsManager::mAnimatedWaterIsEnabled;
	switch (stage->_objectID) {
	case 1:
		*on = dayNight && water;
		return true;
	case 2:
		*on = !dayNight && water;
		return true;
	case 3:
		*on = dayNight && !water;
		return true;
	case 4:
		*on = !dayNight && !water;
		return true;
	case 5:
		*on = dayNight;
		return true;
	case 6:
		*on = !dayNight;
		return true;
	case 7:
		*on = water;
		return true;
	}
	return false;
}

intptr_t GroundElementModel::_reliefStateChanged(char *)
{
	invalidateShaderDependencies();
	if (_element)
		_element->_releaseHeightVBs();
	return 0;
}

void GroundElementModel::changeCurrentLoadedLODTo(int lod)
{
	CAdaptiveFlatMesh::changeCurrentLoadedLODTo(lod);
}

bool GroundElementModel::preloadLODBeforeDraw(int lod)
{
	if (sLargeBBGrabbingIsActive && lod >= 5) {
		GroundElement *element = _element;
		bool clamp = element->_demDisabled || !GroundElementsManager::mDrawWithRelief;
		if (!clamp && !element->_demAvailable) {
			// Load it now: the grab draws once.
			LoadDEMTask *task = LoadDEMTask::_alloc()->initWithElement(element);
			if (task) {
				LTaskQueue *queue = GroundElementsManager::instance()->loadingQueue();
				if (_element->_demLoading)
					queue->removeTaskWithID((int)CTextureLoader::keyForID(_element->_id), 0);
				_element->_demLoading = true;
				sNumPosts++;
				task->executeAndInvokeSelector(0);
				if (_element->_demAvailable)
					changeCurrentLoadedLODTo(lod);
				task->release();
			}
		} else if (clamp) {
			if (lod > 8)
				lod = 8;
			if (lod > _loadedLOD)
				changeCurrentLoadedLODTo(lod);
		} else if (lod > _loadedLOD) {
			changeCurrentLoadedLODTo(lod);
		}
	}
	return CAdaptiveFlatMesh::preloadLODBeforeDraw(lod);
}

void GroundElementModel::analyzeLODRangeAfterDraw(int minLOD, int maxLOD)
{
	_element->_minDrawnLOD = minLOD;
	_element->_maxDrawnLOD = maxLOD;
	GroundElement *element = _element;
	if (maxLOD >= 8 && !element->_demDisabled && !element->_demLoading && !element->_demAvailable &&
	    GroundElementsManager::mDrawWithRelief) {
		LoadDEMTask *task = LoadDEMTask::_alloc()->initWithElement(element);
		if (task) {
			_element->_demLoading = true;
			sNumPosts++;
			if (!sLargeBBGrabbingIsActive) {
				GroundElementsManager *manager = GroundElementsManager::instance();
				manager->loadingQueue()->postTaskToQueue(task);
				manager->markElementAsDEMLoading(_element);
			}
			task->release();
		}
		return;
	}
	if (maxLOD <= 4)
		return;
	if (!_element->_demDisabled && maxLOD > 7 && GroundElementsManager::mDrawWithRelief) {
		if (_element->_demAvailable && _loadedLOD < maxLOD)
			changeCurrentLoadedLODTo(maxLOD);
	} else {
		int lod = maxLOD < 9 ? maxLOD : 8;
		if (lod != _loadedLOD)
			changeCurrentLoadedLODTo(lod);
	}
}
