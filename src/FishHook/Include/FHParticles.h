// Particle systems: particles drawn as points, as quads, or by instancing a
// model once per particle.

#ifndef FHPARTICLES_H
#define FHPARTICLES_H

#include "FHModels.h"

class CParticleSystem;
class CParticleSystemData;

enum TPartSysRepresentationMode {
	kPartSysNone = 0,
	kPartSysPoints = 1,	// a point sprite a particle
	kPartSysQuads = 2,	// four vertices a particle, their corner in the position's w
	kPartSysInstances = 3	// the instance model drawn once a particle
};

// ---------------------------------------------------------------------------
// CParticleSubsystem: the particles [_first, _end) of a system, as many as
// one buffer holds.

class CParticleSubsystem : public CBuffersOwner, public IBuffersOwnerDataSource, public IIndexBufferDataSource {
	LF_DECLARE_CLASS(CParticleSubsystem)
public:
	CParticleSubsystem();
	virtual void dealloc();
	virtual LClass *modelDataClass();
	virtual bool evalVisibilityParams(CDrawableModelData *, Vector3 *, float *) { return true; }
	virtual long beginDrawing(CDrawableModelData *data);
	virtual void drawSubset(CDrawableModelData *data, long subset, long index);
	virtual const void *_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer);
	virtual bool _getTextureForSource(BaseTextureEmu **texture, int source);
	virtual bool _isStageOn(CShaderStage *stage, unsigned char *on);
	virtual bool _getClipPlanes(CClipPlane ***planes, int *count);

	virtual D3DPRIMITIVETYPE primitiveTypeForBufOwner(CBuffersOwner *owner);
	virtual CVertexBuffer *vertexBufferForBufOwner(int stream, CBuffersOwner *owner);
	virtual CIndexBuffer *indexBufferForBufOwner(CBuffersOwner *owner);
	virtual unsigned int indsCountForBuffer(CIndexBuffer *ib);
	virtual CShader *renderShaderForBuffer(CIndexBuffer *ib);
	virtual bool use32BitsForBuffer(CIndexBuffer *ib);
	virtual void fillBuffer(CIndexBuffer *ib);

	CParticleSubsystem *initWithParticlesCount(unsigned int count, unsigned int first, TPartSysRepresentationMode mode,
	                                           CRenderProfile *profile);
	// The subsystems of `cls` (default CParticleSubsystem) for `count` particles.
	static LArray *subsystemsWithMaxParticlesCount(CParticleSystem *system, unsigned int count,
	                                               TPartSysRepresentationMode mode, CRenderProfile *profile,
	                                               LClass *cls);

	unsigned int _count;
	unsigned int _first;
	unsigned int _end;	// the system's particles count while drawing
	TPartSysRepresentationMode _mode;
	CIndexBuffer *_ownIB;
	CParticleSystem *_system;
	unsigned int _index;	// in the system's subsystems
};

// The data a subsystem is drawn with: the system data's.
class CParticleSubsystemData : public ModelDataWithMatrix {
	LF_DECLARE_CLASS(CParticleSubsystemData)
public:
	CParticleSubsystemData() : _systemData(0) {}
	virtual const void *_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer);
	virtual bool _getTextureForSource(BaseTextureEmu **texture, int source);
	virtual bool _isStageOn(CShaderStage *stage, unsigned char *on);
	virtual bool _getClipPlanes(CClipPlane ***planes, int *count);

	CParticleSystemData *_systemData;
};

// ---------------------------------------------------------------------------
// CParticleSystem.

class CParticleSystem : public CDrawableModel, public IVertexBufferDataSource {
	LF_DECLARE_CLASS(CParticleSystem)
public:
	CParticleSystem();
	virtual void dealloc();
	virtual void instantiateSelf() {}
	virtual void releaseSelf() {}
	virtual LClass *modelDataClass();
	virtual CRenderProfile *renderProfile() { return _profile; }
	virtual bool evalVisibilityParams(CDrawableModelData *, Vector3 *, float *) { return true; }
	virtual void doImmediateDrawWithData(CDrawableModelData *data, CFrameDrawer *drawer);
	virtual CInstanceConstDataProvider *_nextInstanceConstDataProvider();
	// With `sharedBuffers` the vertex buffers are the system's and refilled
	// for each data; otherwise each data has its own. `subsystemClass`
	// makes the subsystems of point and quad systems, `instanceModel` is
	// the model of an instancing one.
	virtual CParticleSystem *initWithParams(unsigned char sharedBuffers, unsigned int maxParticles,
	                                        TPartSysRepresentationMode mode, CRenderProfile *profile,
	                                        CBuffersOwner *instanceModel, LClass *subsystemClass);
	virtual unsigned int vertCountForBuffer(CVertexBuffer *vb);
	virtual CShader *renderShaderForBuffer(CVertexBuffer *vb);
	virtual unsigned int streamNumberForBuffer(CVertexBuffer *vb);
	virtual void fillBuffer(CVertexBuffer *) {}
	virtual void _fillParticlesModelVB(CVertexBuffer *vb, CParticleSystemData *data, void *particles, unsigned int count);
	virtual void _fillParticlesModelInstanceVB(CVertexBuffer *vb, CParticleSystemData *data);

	CVertexBuffer *instancingVertexBuffer();
	CVertexBuffer *vertexBufferForSubModelAtIndex(unsigned int index);
	TPartSysRepresentationMode mode() const { return _mode; }
	unsigned int maxParticlesCount() const { return _maxParticles; }
	LArray *subsystems() const { return _subsystems; }

	CRenderProfile *_profile;
	LArray *_subsystems;
	TPartSysRepresentationMode _mode;
	unsigned int _maxParticles;
	bool _sharedBuffers;
	LArray *_vbs;	// shared, one a subsystem
	CBuffersOwner *_instanceModel;
	CVertexBuffer *_instancingVB;	// shared
};

// ---------------------------------------------------------------------------
// CParticleSystemData: the particles of one place a system is drawn.

class CParticleSystemData : public ModelDataWithMatrix, public IVertexBufferDataSource {
	LF_DECLARE_CLASS(CParticleSystemData)
public:
	struct SStoredParticle {
		Vector3 position;
		unsigned int color;
		float size;
	};

	CParticleSystemData();
	virtual void dealloc();
	virtual CDrawableModelData *initWithHostModel(CDrawableModel *model);
	// Subclasses with larger particles start with an SStoredParticle.
	virtual unsigned int particleStructSize() { return sizeof(SStoredParticle); }
	virtual LClass *drawProxyClass();
	virtual unsigned int vertCountForBuffer(CVertexBuffer *vb);
	virtual CShader *renderShaderForBuffer(CVertexBuffer *vb);
	virtual unsigned int streamNumberForBuffer(CVertexBuffer *vb);
	virtual void fillBuffer(CVertexBuffer *vb);

	CParticleSystem *system() const { return (CParticleSystem *)_model; }
	// A new autoreleased proxy that draws this data with its own matrix.
	CDrawableModelData *detachDrawProxy();
	unsigned int maxParticlesCount() const { return system()->_maxParticles; }
	unsigned int particlesCount() const { return _count; }
	unsigned int setParticlesCount(unsigned int count);
	void *particles() const { return _particles; }
	SStoredParticle *particleAtIndex(unsigned int i)
	{
		return (SStoredParticle *)((char *)_particles + i * particleStructSize());
	}
	// Call after changing the particles.
	void invalidateParticles() { _vbsDirty = _instancingDirty = true; }
	CVertexBuffer *instancingVertexBuffer();
	CVertexBuffer *vertexBufferForSubModelAtIndex(unsigned int index);

	LArray *_subDatas;	// a CParticleSubsystemData a subsystem
	void *_particles;
	unsigned int _count;
	bool _vbsDirty;
	bool _instancingDirty;
	LArray *_vbs;	// unshared, one a subsystem
	CVertexBuffer *_instancingVB;	// unshared
};

class CParticleSystemDrawProxy : public ModelDataWithMatrix {
	LF_DECLARE_CLASS(CParticleSystemDrawProxy)
public:
	CParticleSystemDrawProxy() : _systemData(0) {}
	virtual void dealloc();
	virtual void immediateDraw(CFrameDrawer *drawer);
	virtual CParticleSystemDrawProxy *initWithPSystemData(CParticleSystemData *data);
	virtual void _copyAttributesToHost();

	CParticleSystemData *_systemData;
};

#endif
