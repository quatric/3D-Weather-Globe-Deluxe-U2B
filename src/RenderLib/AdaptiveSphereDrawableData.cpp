// The globe's coarse patches of a frame, sorted by texture, with the fog and
// the transparency.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(AdaptiveSphereDrawableData, LF_CLASSOF(ModelDataWithMatrix))

AdaptiveSphereDrawableData::AdaptiveSphereDrawableData() : _fogClip(0.0f), _alpha(1.0f)
{
	_patches.setComparator(_comparePatchInfoByTexture);
	_patches.setKeepsSorted(true);
}

bool AdaptiveSphereDrawableData::_isStageOn(CShaderStage *stage, unsigned char *on)
{
	*on = stage->_objectID != 2 || _alpha < 0.98f;
	return true;
}

int AdaptiveSphereDrawableData::_comparePatchInfoByTexture(const void *a, const void *b, const void *)
{
	const STextureID &ta = ((const AdaptiveLODSphere::SPatchDrawInfo *)a)->texture.textureID;
	const STextureID &tb = ((const AdaptiveLODSphere::SPatchDrawInfo *)b)->texture.textureID;
	return (int)(CTextureLoader::keyForID(tb) - CTextureLoader::keyForID(ta));
}

const void *AdaptiveSphereDrawableData::_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer)
{
	if (descr->id == 'FPGl') {
		CContext *context = _model->_context;
		if (context)
			UMathUtils::fillExp2FogParamsConstant((float *)buffer, context->worldViewMatrixTransp(), _fogClip * 0.4f,
			                                      _fogClip, 2.1f);
		return buffer;
	}
	if (descr->id == 'FPGm') {
		float *v = (float *)buffer;
		v[0] = 1.0f;
		v[1] = 1.0f;
		v[2] = 1.0f;
		v[3] = _alpha;
		return buffer;
	}
	return CDrawableModelData::_dataPtrForConstant(descr, buffer);
}

void AdaptiveSphereDrawableData::addElement(const STextureID &id, int textureLevel)
{
	AdaptiveLODSphere::SPatchDrawInfo info(id, 0);
	CTextureLoader *loader = (CTextureLoader *)_model->_holder->objectForKey(kTextureLoaderKey);
	info.adjustWithLoader(textureLevel, loader);
	_patches.addObject(info);
}

// The globe fades in with `visibility` and is as opaque as the preferences say.
void AdaptiveSphereDrawableData::startFrame(float farClip, float visibility)
{
	_patches.removeAllObjects();
	_fogClip = AdaptiveLODSphere::calcFogClip(farClip);
	float fade = visibility * 8.0f - 2.5f;
	if (fade > 1.0f)
		fade = 1.0f;
	if (!(fade >= 0.0f))
		fade = 0.0f;
	float transparency = clamp01(Preferences::globeTransparencyLevel());
	_alpha = fade + (1.0f - transparency) * (1.0f - fade);
}
