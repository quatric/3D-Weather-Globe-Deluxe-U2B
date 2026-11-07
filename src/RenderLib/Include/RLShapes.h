// RenderLib's shape containers and drawables: the polygons of countries,
// states, counties and time zones as outlines (line lists over the index
// BLOB's polylines) and as fills (triangulated with triangulate()).

#ifndef RLSHAPES_H
#define RLSHAPES_H

#include "AppModel.h"
#include "LObjToNSObjWrapper.h"

// ---------------------------------------------------------------------------
// Triangle's interface (triangle.cpp). REAL is float, as the original built it.

struct triangulateio {
	float *pointlist;
	float *pointattributelist;
	int *pointmarkerlist;
	int numberofpoints;
	int numberofpointattributes;

	int *trianglelist;
	float *triangleattributelist;
	float *trianglearealist;
	int *neighborlist;
	int numberoftriangles;
	int numberofcorners;
	int numberoftriangleattributes;

	int *segmentlist;
	int *segmentmarkerlist;
	int numberofsegments;

	float *holelist;
	int numberofholes;

	float *regionlist;
	int numberofregions;

	int *edgelist;
	int *edgemarkerlist;
	float *normlist;
	int numberofedges;
};

// Triangulates the polygon in `in` (its points and the segments closing them
// into a ring) into `out`: the same points and the triangles, zero-based and
// counter-clockwise, in malloc'd lists the caller frees.
void triangulate(const char *switches, triangulateio *in, triangulateio *out, triangulateio *vorout);

class CPolysDrawable;

// ---------------------------------------------------------------------------
// CPolysContainer: polygons read from the index BLOB.

class CPolysContainer : public CShapeContainer {
	LF_DECLARE_CLASS(CPolysContainer)
public:
	// Walks the index BLOB: big-endian LOD count, then per LOD its number and
	// polygon count, then per polygon its number, the vertex BLOB's 10-char
	// UID, the chunk, the index count and the big-endian 16-bit indices.
	class _polyLineIterator {
	public:
		_polyLineIterator(IndexBuffersBLOB *indexes);
		~_polyLineIterator();
		bool nextLOD();
		bool nextPoly();

		int _lodCount;
		int _lod;
		int _poly;
		const char *_uid;
		int _chunk;
		int _count;
		const uint16_t *_indices;
		LData *_data;
		const unsigned char *_cursor;
		int _polyCount;
	};

	CPolysContainer();
	virtual void dealloc();
	virtual CPolysContainer *getOwnerLObject() { return this; }
	virtual bool hitTest(LPoint geoPoint);
	virtual LRect allPathsBoundsGeoRect();
	virtual CShapeDrawable *drawableForProjection(CProjection *projection, int mode, float scale);
	virtual LClass *drawableClass();
	virtual bool _loadShapes();

	LRect largestPathBoundGeoRect();
	// LObjToNSObjWrapper's of NSBezierPath, one per polygon of the finest LOD.
	LArray *allPaths();
	// Debug: writes the LOD's polygons as "count\n" then "x y\n" lines.
	bool writeToFile(int lod, const char *path);
	// The LOD's polygons as paths, with their bounds (LRects appended to
	// `bounds`), the bounds of all (across the date line, the narrowest of
	// the three placements) and the largest.
	bool _loadPathsForLod(int lod, LArray *paths, LData *bounds, LRect *allBounds, LRect *largestBounds);
	LObjToNSObjWrapper *_createPathWithPoints(LPoint *points, int count);
	bool _isGeoPointVisibleForPath(LPoint geoPoint, LObjToNSObjWrapper *path);

	LArray *_paths;
	LData *_bounds;
	LRect _allBounds;
	LRect _largestBounds;
};

// A list of triangle batches drawn with one vertex buffer (at most 0xffff
// vertices).
struct SBuffersList {
	struct Node {
		Node *next;
		int vertCount;
		int triCount;
		LPoint *points;	// new[], three per triangle
		int *indices;	// new[] or 0 (then 0, 1, 2...)
	};
	Node *head;
	int vertCount;
	int triCount;
};

class CPolysDrawable : public CShapeDrawable, public IIndexBufferDataSource {
	LF_DECLARE_CLASS(CPolysDrawable)
public:
	struct SPolygonParams {
		int count;
		const uint16_t *indices;	// big-endian
	};
	// The outlines drawn with one vertex buffer chunk.
	struct SSingleModelParams {
		const char *uid;
		int chunk;
		int count;
		TArray<SPolygonParams> *polygons;
		CIndexBuffer *ib;
	};

	CPolysDrawable();
	virtual void dealloc();
	virtual LArray *drawableDatas(int index, CHolder *holder, const LString *usageKey, unsigned char *created);
	virtual LString *_renderProfileFile();
	virtual int _modelsCount();
	virtual LClass *_classForModel(unsigned int index);
	virtual void _modelWasCreated(CBuffersOwner *model, unsigned int index);
	virtual CPolysDrawable *getOwnerLObject() { return this; }
	virtual CVertexBuffer *vertexBufferForBufOwner(int stream, CBuffersOwner *owner);
	virtual D3DPRIMITIVETYPE primitiveTypeForBufOwner(CBuffersOwner *owner);
	virtual CIndexBuffer *indexBufferForBufOwner(CBuffersOwner *owner);
	virtual unsigned int indsCountForBuffer(CIndexBuffer *ib);
	virtual CShader *renderShaderForBuffer(CIndexBuffer *ib);
	virtual bool use32BitsForBuffer(CIndexBuffer *ib) { return false; }
	virtual void fillBuffer(CIndexBuffer *ib);

	void setTriangleEdgeLeng(float length) { _maxEdgeSq = length * length; }
	int pathsCount();
	void _parseIBIfNeeded();
	// The fill's buffer lists, made once; returns their number.
	int _triangulate();
	// Splits the triangle on its longest side until no side is longer than
	// the edge length; counts the pieces, and stores them when _tris is set.
	void _subdivideTriangle(LPoint &a, LPoint &b, LPoint &c);

	float _maxEdgeSq;
	TArray<int> _splitCounts;
	int _triCount;
	LPoint *_tris;
	TArray<SBuffersList *> *_buffers;
	bool _triangulated;
	bool _ibParsed;
	TArray<SSingleModelParams> *_modelParams;
	LArray *_paths;
};

// The fill's vertices: the nodes' points through the projection.
class VBFilledPolygonDataSource : public LObject, public IVertexBufferDataSource {
	LF_DECLARE_CLASS(VBFilledPolygonDataSource)
public:
	VBFilledPolygonDataSource();
	virtual VBFilledPolygonDataSource *getOwnerLObject() { return this; }
	virtual unsigned int vertCountForBuffer(CVertexBuffer *vb) { return _vertCount; }
	virtual CShader *renderShaderForBuffer(CVertexBuffer *vb);
	virtual unsigned int streamNumberForBuffer(CVertexBuffer *vb) { return 0; }
	virtual void fillBuffer(CVertexBuffer *vb);

	CRenderProfile *_profile;	// not retained
	CProjection *_projection;	// not retained
	SBuffersList::Node *_head;
	int _vertCount;
};

// The index buffers of the outlines and of the fill, with their model's index.
class PolygonIndexBuffer : public CIndexBuffer {
	LF_DECLARE_CLASS(PolygonIndexBuffer)
public:
	PolygonIndexBuffer() : _modelIndex(0) {}
	virtual PolygonIndexBuffer *getOwnerLObject() { return this; }

	int _modelIndex;
};

class FilledPolygonIndexBuffer : public CIndexBuffer {
	LF_DECLARE_CLASS(FilledPolygonIndexBuffer)
public:
	FilledPolygonIndexBuffer() : _modelIndex(0) {}
	virtual FilledPolygonIndexBuffer *getOwnerLObject() { return this; }

	int _modelIndex;
};

class PolygonDrawableModel : public CBuffersOwner {
	LF_DECLARE_CLASS(PolygonDrawableModel)
public:
	PolygonDrawableModel() : _index(0) {}
	virtual LClass *modelDataClass();
	virtual bool evalVisibilityParams(CDrawableModelData *data, Vector3 *center, float *radius);
	virtual PolygonDrawableModel *getOwnerLObject() { return this; }
	virtual void delloc() { CBuffersOwner::dealloc(); }

	int _index;
};

class FilledPolygonDrawableModel : public CBuffersOwner {
	LF_DECLARE_CLASS(FilledPolygonDrawableModel)
public:
	FilledPolygonDrawableModel() : _index(0) {}
	virtual LClass *modelDataClass();
	virtual bool evalVisibilityParams(CDrawableModelData *data, Vector3 *center, float *radius);
	virtual FilledPolygonDrawableModel *getOwnerLObject() { return this; }
	virtual void delloc() { CBuffersOwner::dealloc(); }

	int _index;
};

// The outline's stage 2 is on for every usage, and the others for the hit
// shape's highlight; the fill draws its stage 3.
class PolygonDrawableModelData : public ShapeModelData {
	LF_DECLARE_CLASS(PolygonDrawableModelData)
public:
	PolygonDrawableModelData() : _hitShape(false) {}
	virtual PolygonDrawableModelData *getOwnerLObject() { return this; }
	virtual void setupWithUsageKey(const LString *key);
	virtual bool _isStageOn(CShaderStage *stage, unsigned char *on);

	bool _hitShape;
};

class FilledPolygonDrawableModelData : public ShapeModelData {
	LF_DECLARE_CLASS(FilledPolygonDrawableModelData)
public:
	FilledPolygonDrawableModelData() : _hitShape(false) {}
	virtual FilledPolygonDrawableModelData *getOwnerLObject() { return this; }
	virtual void setupWithUsageKey(const LString *key);
	virtual bool _isStageOn(CShaderStage *stage, unsigned char *on);

	bool _hitShape;
};

// ---------------------------------------------------------------------------
// Lines and points: declared by the original, never loaded (the shape manager
// makes only CPolysContainers).

class CLinesContainer : public CShapeContainer {
	LF_DECLARE_CLASS(CLinesContainer)
public:
	virtual CLinesContainer *getOwnerLObject() { return this; }
};

class CLinesDrawable : public CShapeDrawable {
	LF_DECLARE_ABSTRACT_CLASS(CLinesDrawable)
};

class CPointsContainer : public CShapeContainer {
	LF_DECLARE_CLASS(CPointsContainer)
public:
	virtual CPointsContainer *init();
	virtual CPointsContainer *getOwnerLObject() { return this; }
};

class CPointsDrawable : public CShapeDrawable {
	LF_DECLARE_ABSTRACT_CLASS(CPointsDrawable)
};

#endif
