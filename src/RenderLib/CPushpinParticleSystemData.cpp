// CPushpinParticleSystemData: the route points' pushpins, each turned to
// stand on the globe at its particle's position.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(CPushpinParticleSystemData, LF_CLASSOF(CParticleSystemData))

void CPushpinParticleSystemData::dealloc()
{
	releaseAndClear(_transforms);
	CParticleSystemData::dealloc();
}

// A pushpin's height above the ground while jumping: bounces that fade and
// return, out of step from one pushpin to the next.
float CPushpinParticleSystemData::_pushpinAnimationFactor(int index, double time)
{
	double t = time * 0.35;
	float phase = (float)(t - floor(t)) + (float)index * 1.1141f;
	phase -= floorf(phase);
	float amplitude = (1.0f - phase) * 0.7f * (1.0f - phase) + 0.3f;
	float bounce = phase / (amplitude * 0.4f);
	bounce = (bounce - floorf(bounce)) * 3.0f;
	if (bounce > 1.0f)
		bounce = (3.0f - bounce) * 0.5f;
	float fade = amplitude * 1.3f;
	if (fade > 1.0f)
		fade = 1.0f;
	return (1.0f - (1.0f - bounce) * (1.0f - bounce)) * fade * fade * 0.035f;
}

const void *CPushpinParticleSystemData::_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer)
{
	if (descr->id != 'pppm')
		return CDrawableModelData::_dataPtrForConstant(descr, buffer);
	float *v = (float *)buffer;
	v[0] = _scale;
	v[1] = _fogVisibility;
	v[2] = 1.0f - _fogVisibility;
	v[3] = 0.0f;
	return buffer;
}

void CPushpinParticleSystemData::storeSceneStateInStream(LData *stream, unsigned int flags)
{
	ModelDataWithMatrix::storeSceneStateInStream(stream, flags);
	CPushpinParticleSystem *system = lobj_cast<CPushpinParticleSystem>(_model);
	storeFloatInStream(_scale, stream);
	storeIntInStream(system->_jumpingMarkers, stream);
}

const void *CPushpinParticleSystemData::restoreSceneStateFromData(const void *data, unsigned int flags)
{
	const void *next = ModelDataWithMatrix::restoreSceneStateFromData(data, flags);
	CPushpinParticleSystem *system = lobj_cast<CPushpinParticleSystem>(_model);
	next = loadFloatFromStream(next, &_scale);
	int jumping;
	next = loadIntFromStream(next, &jumping);
	system->_jumpingMarkers = jumping != 0;
	if (jumping)
		_instancingDirty = true;
	return next;
}

// Rows of the rotation with the position, lifted while jumping, then the
// colour.
const void *CPushpinParticleSystemData::_instanceDataPtrForConstant(unsigned int index, void *buffer)
{
	CPushpinParticleSystem *system = lobj_cast<CPushpinParticleSystem>(_model);
	unsigned int size = particleStructSize();
	float lift = 1.0005f;
	if (system->_jumpingMarkers)
		lift = _pushpinAnimationFactor(index, LTimer::currentTimeValue()) + 1.0f;
	const SStoredParticle *particle = (const SStoredParticle *)((char *)_particles + size * index);
	const float *m = (const float *)_transforms->bytes() + index * 9;
	float *v = (float *)buffer;
	v[0] = m[0];
	v[1] = m[1];
	v[2] = m[2];
	v[3] = particle->position.x * lift;
	v[4] = m[3];
	v[5] = m[4];
	v[6] = m[5];
	v[7] = particle->position.y * lift;
	v[8] = m[6];
	v[9] = m[7];
	v[10] = m[8];
	v[11] = lift * particle->position.z;
	Color color(particle->color);
	v[12] = color.r;
	v[13] = color.g;
	v[14] = color.b;
	v[15] = color.a;
	return buffer;
}

// Turns +z to point down to the centre of the globe, then flips it out.
void CPushpinParticleSystemData::updateTransformsData()
{
	if (!_vbsDirty)
		return;
	releaseAndClear(_transforms);
	_transforms = (LData *)LData::_alloc()->init();
	_transforms->setLength(_count * 0x24);
	float *m = (float *)_transforms->bytes();
	unsigned int size = particleStructSize();
	Vector3 z(0.0f, 0.0f, 1.0f);
	for (unsigned int i = 0; i < _count; i++, m += 9) {
		Vector3 p = ((const SStoredParticle *)((char *)_particles + i * size))->position;
		Vector3 axis;
		FHVec3Cross(&axis, &z, &p);
		_SQuater q;
		FHQuaternionRotationAxis(&q, &axis, (float)(acos((double)-p.z) + 3.1415927410125732));
		Matrix r;
		FHMatrixRotationQuaternion(&r, &q);
		m[0] = r._11;
		m[1] = r._12;
		m[2] = r._13;
		m[3] = r._21;
		m[4] = r._22;
		m[5] = r._23;
		m[6] = r._31;
		m[7] = r._32;
		m[8] = r._33;
	}
	_vbsDirty = false;
}
