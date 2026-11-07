// The logo made of points.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(LogoSystem, 0)

void LogoSystem::dealloc()
{
	releaseAndClear(_texturePath);
	releaseAndClear(_data);
	LObject::dealloc();
}

LogoSystem *LogoSystem::initWithParams(CHolder *holder, LString *texturePath, int columns)
{
	if (!LObject::init())
		return 0;
	_texturePath = LRetain(texturePath);
	_data = 0;
	_aspect = 1.0f;
	_holder = holder;
	_columns = columns;
	return this;
}

CParticleSystemData *LogoSystem::modelData()
{
	if (_data)
		return _data;
	C2DTexture *logo = C2DTexture::textureWithContentsOfFile(_texturePath, _holder);
	if (!logo)
		return 0;
	int columns = _columns;
	unsigned int rows = (unsigned int)(columns * logo->_info.Height) / logo->_info.Width;
	_aspect = (float)columns / (float)(int)rows;
	CRenderProfile *profile =
	    CRenderProfile::renderProfileByFileDesc(LString::stringWithCString("RENDERPROFILES\\LogoPoints.yaf"), _holder);
	CHolder *holder = _holder;
	if (holder)
		holder->beginAccumulationBlock();
	CParticleSystem *system = CParticleSystem::_alloc();
	system->initWithParams(0, columns * rows, kPartSysQuads, profile, 0, 0);
	_data = LRetain(lobj_cast<CParticleSystemData>(system->detachModelDataInstance()));
	system->setTextureForSource(logo, 0);

	// Each point lies on the ray from the camera at (0, -1, 0) through its
	// place on the logo, at a random depth; its color is its texture
	// coordinate.
	CParticleSystemData::SStoredParticle *p = (CParticleSystemData::SStoredParticle *)_data->particles();
	int v = 0;
	for (int row = 0; row < (int)rows; row++) {
		int u = 0;
		for (int column = 0; column < columns; column++) {
			Vector3 dir(((float)column / (float)(columns - 1) - 0.5f) * 0.5f, 1.0f,
			            ((2.5f - (float)row / (float)(int)(rows - 1)) / _aspect) * 0.5f);
			FHVec3Normalize(&dir, &dir);
			float depth = (float)rand() * 4.656613e-10f + 0.0f;
			depth = depth * depth * 15.0f + 0.15f;
			p->position = Vector3(depth * dir.x + 0.0f, depth * dir.y - 1.0f, depth * dir.z + 0.0f);
			p->size = 0.011f;
			p->color = (unsigned int)((u / columns) & 0xff) | (unsigned int)((v / (int)rows) & 0xff) << 8;
			p++;
			u += 0xff;
		}
		v += 0xff;
	}
	C2DTexture *star = C2DTexture::textureWithContentsOfFile(LString::stringWithCString("TEXTURES\\star_max.dds"), _holder);
	system->setTextureForSource(star, 1);
	system->release();
	_data->_vbsDirty = true;
	_data->_instancingDirty = true;
	if (holder)
		holder->endAccumulationBlock();
	return _data;
}

float LogoSystem::aspect()
{
	modelData();
	return _aspect;
}
