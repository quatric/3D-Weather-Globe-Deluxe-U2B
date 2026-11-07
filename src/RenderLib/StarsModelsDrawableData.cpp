// The stars' data: the catalog patches in view and the magnitudes drawn.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(StarsModelsDrawableData, LF_CLASSOF(CParticleSystemData))

StarsModelsDrawableData::StarsModelsDrawableData() : _magnitude(10.0f, 5.0f, 0.0f, 0.0f)
{
}

// The level 3 patches (8 x 16) whose bounding spheres are in view.
void StarsModelsDrawableData::fillVisibleEntries(CContext *context)
{
	_visiblePatches.removeAllObjects();
	float radius = sinf(0.175f);
	for (int y = 0; y < 8; y++) {
		float a = ((float)y + 0.5f) * 0.125f * 3.1415927f;
		float ca = cosf(a);
		double sa = sin((double)a);
		for (int x = 0; x < 16; x++) {
			double b = (double)(((float)x + 0.5f) * 0.125f * 3.1415927f);
			Vector3 center((float)(sa * cos(b)), (float)(sin(b) * sa), -ca);
			if (context->sphereProbe(&center, radius, worldMatrix(), 0, 0)) {
				AtlasDatabase::SStarsPathID id;
				id.x = (char)x;
				id.y = (char)y;
				id.level = 3;
				id.pad = 0;
				_visiblePatches.addObject(id);
			}
		}
	}
}

const void *StarsModelsDrawableData::_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer)
{
	if (descr->id == 'CnSm')
		return &_magnitude;
	if (descr->id != 'CnSn')
		return CDrawableModelData::_dataPtrForConstant(descr, buffer);
	// The quad corners, square on the screen.
	LSize size = _profile->holder()->relativeFrameSize();
	float a = (size.height / size.width) * 0.009f;
	float *v = (float *)buffer;
	const float corners[4][2] = {{-a, -0.009f}, {a, -0.009f}, {-a, 0.009f}, {a, 0.009f}};
	for (int i = 0; i < 4; i++) {
		v[i * 4] = corners[i][0];
		v[i * 4 + 1] = corners[i][1];
		v[i * 4 + 2] = 0.0f;
		v[i * 4 + 3] = 0.0f;
	}
	return buffer;
}
