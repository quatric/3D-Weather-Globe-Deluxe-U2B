// Meshes: models whose vertices and indices a data source fills once, the
// standard shapes, and the adaptive (level of detail) flat meshes.

#ifndef FHMESHES_H
#define FHMESHES_H

#include "FHModels.h"

class CMesh;
class CAdaptiveFlatMesh;
class CMeshLODEntry;

// ---------------------------------------------------------------------------
// CMesh

class CMeshDataSource {
public:
	virtual D3DPRIMITIVETYPE primitiveTypeForMesh(CMesh *mesh) = 0;
	virtual unsigned int numPrimitivesForMesh(CMesh *mesh) = 0;
	virtual unsigned int numVerticesForMesh(CMesh *mesh) = 0;
	// Fill the buffers here.
	virtual void meshWasInstantiated(CMesh *mesh) = 0;
	virtual void meshWillRelease(CMesh *) {}
	virtual void meshWillDie(CMesh *) {}
	virtual ~CMeshDataSource() {}
};

// A range of a mesh drawn as one subset (D3DXATTRIBUTERANGE).
struct SMeshAttributeRange {
	unsigned int attribId;
	unsigned int faceStart;
	unsigned int faceCount;
	unsigned int vertexStart;
	unsigned int vertexCount;
};

class CMesh : public CBuffersOwner, public IBuffersOwnerDataSource {
	LF_DECLARE_CLASS(CMesh)
public:
	CMesh();
	virtual void dealloc();
	virtual void instantiateSelf();
	virtual void releaseSelf();
	virtual LClass *modelDataClass();
	virtual bool evalVisibilityParams(CDrawableModelData *data, Vector3 *center, float *distance);
	virtual void doImmediateDrawWithData(CDrawableModelData *data, CFrameDrawer *drawer);
	virtual void drawSubset(CDrawableModelData *data, long subset, long index);
	virtual void setPerformBoxVisibilityTest(unsigned char perform) { _performBoxVisibilityTest = perform; }
	virtual CVertexBuffer *vertexBufferForBufOwner(int stream, CBuffersOwner *owner);
	virtual D3DPRIMITIVETYPE primitiveTypeForBufOwner(CBuffersOwner *) { return _meshPrimType; }
	virtual CIndexBuffer *indexBufferForBufOwner(CBuffersOwner *) { return _meshIB; }
	virtual void createMesh();
	virtual void updateBoundingParameters();
	virtual void drawMeshFragment(unsigned int vertexStart, unsigned int vertexCount, unsigned int faceStart,
	                              unsigned int faceCount);

	// The source is retained when `retain` is set and it is an LObject.
	CMesh *initWithDataSource(CMeshDataSource *source, CRenderProfile *profile, unsigned char retain);
	CMesh *initWithStdGeometry(const struct SStdGeometry *geometry, CRenderProfile *profile);
	void dataSourceWillDie();
	void setMeshOptions(unsigned int options) { _meshOptions = options; }
	DXMeshEmu *dxMesh() const { return _dxMesh; }
	const Vector3 *boxMin() const { return &_boxMin; }
	const Vector3 *boxMax() const { return &_boxMax; }

	unsigned int _meshOptions;	// D3DXMESH flags added to the computed ones
	D3DPRIMITIVETYPE _meshPrimType;
	DXMeshEmu *_dxMesh;
	CVertexBuffer *_meshVB;
	CIndexBuffer *_meshIB;
	void *_extra;	// deleted with the mesh
	unsigned int _attributesCount;
	SMeshAttributeRange *_attributes;
	bool _softwareProcessing;
	CMeshDataSource *_meshDS;
	LObject *_meshDSOwner;
	bool _flag114;
	bool _performBoxVisibilityTest;
	Vector3 _boxMin;
	Vector3 _boxMax;
	int _unused130;
	int _currentSubset;	// -1 before each draw
	bool _flag138;
};

// ---------------------------------------------------------------------------
// Standard shapes: a grid of cols x rows cells.

enum { kStdGeometrySphere = 0, kStdGeometrySpherePart = 1, kStdGeometryCylinder = 2, kStdGeometryPlaneXY = 3 };

struct SStdGeometry {
	int type;
	unsigned short cols, rows;
	float radius;	// the width of a plane
	union {
		float height;	// cylinders and planes
		struct {
			// A sphere part is part (partX, partY) of a sphere cut in partsX x partsY.
			unsigned short partsX, partsY, partX, partY;
		};
	};
};

class CStdGeometryDataSource : public CMeshDataSource {
public:
	CStdGeometryDataSource() { memset(&_geometry, 0, sizeof(_geometry)); }
	virtual D3DPRIMITIVETYPE primitiveTypeForMesh(CMesh *) { return D3DPT_TRIANGLELIST; }
	virtual unsigned int numPrimitivesForMesh(CMesh *mesh);
	virtual unsigned int numVerticesForMesh(CMesh *mesh);
	virtual void meshWasInstantiated(CMesh *mesh);

	void _fillAsCylinder(CMesh *mesh, CRenderProfile *profile, const SStdGeometry *g);
	void _fillAsSphere(CMesh *mesh, CRenderProfile *profile, const SStdGeometry *g);
	void _fillAsSpherePart(CMesh *mesh, CRenderProfile *profile, const SStdGeometry *g);
	void _fillAsPlaneXY(CMesh *mesh, CRenderProfile *profile, const SStdGeometry *g);

	SStdGeometry _geometry;
};

class LStdGeometryDataSource : public LObject, public CStdGeometryDataSource {
	LF_DECLARE_CLASS(LStdGeometryDataSource)
public:
	// Autoreleased.
	static LStdGeometryDataSource *sourceWithStdGeometry(const SStdGeometry *geometry);
};

// ---------------------------------------------------------------------------
// Adaptive flat meshes: a square grid drawn with finer levels of detail
// where it is near. LOD 2n is a 2^n grid; 2n+1 also has the cell centers.
// Patches are numbered by level (a 2^level square of them) and position.

class IAdaptiveMeshDataSource {
public:
	// The detail levels: the mesh goes to LOD 2 * levels.
	virtual char logGridFullSize() = 0;
	virtual CRenderProfile *renderProfile() = 0;
	virtual CVertexBuffer *vertexBufferForLOD(int stream, int lod) = 0;
	// The bounds of every patch, indexed as UAdaptiveMeshUtils lays them out.
	virtual void getMinMaxCorners(Vector2 **mins, Vector2 **maxs, float **minZ, float **maxZ) = 0;
	virtual CSharableResource *boundsMutex() { return 0; }
	virtual void boundsWillBeRequestedForLevel(int) {}
	virtual ~IAdaptiveMeshDataSource() {}

	// The patches of the first `levels` levels.
	static int boundsArrayElementsCount(int levels);
};

struct SSubpatchInfo {
	int startIndex;
	int primCount;
};

class UAdaptiveMeshUtils {
public:
	UAdaptiveMeshUtils();
	// The index range of patch (x, y) of `level` in the index buffer of `lod`.
	static SSubpatchInfo subpatchInfoForLevelAtLOD(int x, int y, int level, int lod);

	static int _offsetForSubEntries[11];	// the first patch of each level
};

class CAdaptiveMeshRLODStreamFiller {
public:
	// The vertex stream telling the shader each vertex's LOD.
	static void fillRLODStream_2n(long size, char *data, long stride, long lodBase);
	static void fillRLODStream_2n_plus_1(long size, char *data, long stride, long lodBase);
};

template <typename T>
class TAdaptiveIBFiller {
public:
	static void fillBorders(T *p, T rowStride, T base, int size);
	static void fillIndices_LOD_2n(T *p, long size, long x, long y, long rowStride, unsigned char borders);
	static void fillIndices_LOD_2n_plus_1(T *p, long size, long x, long y, long rowStride, unsigned char borders);
};

// The buffers of a LOD, shared by every mesh of a holder.
class CADMeshDataSource : public LObject, public IIndexBufferDataSource, public IVertexBufferDataSource {
	LF_DECLARE_CLASS(CADMeshDataSource)
public:
	CADMeshDataSource() : _lod(0), _primCount(0), _vertCount(0), _profile(0) {}
	virtual void dealloc();
	virtual unsigned int vertCountForBuffer(CVertexBuffer *) { return _vertCount; }
	virtual CShader *renderShaderForBuffer(CVertexBuffer *) { return _profile ? _profile->_shader : 0; }
	virtual unsigned int streamNumberForBuffer(CVertexBuffer *vb);
	virtual void fillBuffer(CVertexBuffer *vb);
	virtual bool isSoftwareVertexBuffer(CVertexBuffer *vb);
	virtual unsigned int indsCountForBuffer(CIndexBuffer *) { return _primCount * 3; }
	virtual CShader *renderShaderForBuffer(CIndexBuffer *) { return _profile ? _profile->_shader : 0; }
	virtual bool use32BitsForBuffer(CIndexBuffer *) { return _vertCount > 0xffff; }
	virtual void fillBuffer(CIndexBuffer *ib);

	CADMeshDataSource *initWithLOD(int lod, CRenderProfile *profile);

	int _lod;
	int _primCount;
	int _vertCount;
	CRenderProfile *_profile;
};

class CMeshLODEntry : public LObject {
	LF_DECLARE_CLASS(CMeshLODEntry)
public:
	CMeshLODEntry() : _lod(0), _numVertices(0), _mesh(0), _holder(0), _ds(0), _stateBlock(0) {}
	virtual void dealloc();
	virtual CMeshLODEntry *initWithParams(int lod, CAdaptiveFlatMesh *mesh, IAdaptiveMeshDataSource *ds);
	virtual void invalidateShaderDependencies();
	virtual void immediateDrawPatch(int level, int x, int y, unsigned char borders);
	virtual void setupBuffers();

	CIndexBuffer *sharedIndexBuffer();
	CVertexBuffer *sharedRLODBuffer();

	static CMeshLODEntry *sPrevDrawnEntry;	// whose buffers are set
	static LString *sIBKeys[20];
	static LString *sRLODKeys[20];

	int _lod;
	int _numVertices;
	CAdaptiveFlatMesh *_mesh;
	CHolder *_holder;
	IAdaptiveMeshDataSource *_ds;
	CStateBlock *_stateBlock;	// sets the buffers
};

typedef void (*TAdaptiveMeshTransformCallback)(Vector3 *min, Vector3 *max, CDrawableModelData *data);

class CAdaptiveFlatMesh : public CBuffersOwner {
	LF_DECLARE_ABSTRACT_CLASS(CAdaptiveFlatMesh)
public:
	struct SDrawLodParams {
		CDrawableModelData *data;
		const Matrix *world;
		const Matrix *worldView;
		bool axisAligned;	// the world matrix only scales and moves
		float lodScale;
		Vector2 *mins;
		Vector2 *maxs;
		float *minZ;
		float *maxZ;
	};

	CAdaptiveFlatMesh();
	virtual void dealloc();
	virtual void instantiateSelf();
	virtual void releaseSelf();
	virtual void purgeSelf() { invalidateShaderDependencies(); }
	virtual const void *_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer);
	virtual void bindBuffers() {}
	virtual void drawSubset(CDrawableModelData *data, long subset, long index);
	virtual CAdaptiveFlatMesh *initWithDataSource(IAdaptiveMeshDataSource *ds, CRenderProfile *profile,
	                                              unsigned char retain);
	virtual void setLODBias(float bias) { _lodBias = bias; }
	virtual void setTransformCallback(TAdaptiveMeshTransformCallback callback) { _transformCallback = callback; }
	virtual void invalidateShaderDependencies();
	virtual LClass *meshLODEntryClass();
	virtual void changeCurrentLoadedLODTo(int lod);
	virtual CMeshLODEntry *createMeshForLOD(int lod, unsigned char last);
	virtual bool preloadLODBeforeDraw(int) { return false; }
	virtual void analyzeLODRangeAfterDraw(int, int) {}

	CMeshLODEntry *entryForLOD(int lod);
	void drawLOD(const SDrawLodParams *params, int level, int x, int y, int intersection);

	IAdaptiveMeshDataSource *_ads;
	LObject *_adsOwner;
	float _lodBias;
	signed char _minLOD;
	signed char _loadedLOD;
	bool _drawBorders;
	signed char _maxLOD;
	LArray *_entries;	// LOD 4 and up
	signed char _maxDrawnLOD;
	signed char _minDrawnLOD;
	TAdaptiveMeshTransformCallback _transformCallback;
	CShader *_shaderBound;
};

#endif
