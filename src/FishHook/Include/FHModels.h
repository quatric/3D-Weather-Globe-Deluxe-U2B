// Drawable models, their per-instance data, vertex and index buffers, the
// draw queues and the frame drawer.

#ifndef FHMODELS_H
#define FHMODELS_H

#include "FHRender.h"

class CDrawableModel;
class CDrawableModelData;
class CFrameDrawer;
class CVertexBuffer;
class CIndexBuffer;
class CBuffersOwner;
class CParticleSystemData;
class CScenePropertiesSet;

// The draw orders of the frame drawer. Order 0 is sorted into the opaque,
// transparent and glow queues; the others are drawn as given.
typedef int TOrder;

// An environment map a frame renders into before it is drawn. Nothing in
// this program makes one.
class CEnvironmentMap : public CContextObject {
	LF_DECLARE_ABSTRACT_CLASS(CEnvironmentMap)
public:
	CEnvironmentMap(TCtxObjPriority priority = 0) : CContextObject(priority) {}
	// Returns whether the map needs drawing; then sets it as the target.
	virtual bool beginUpdate() = 0;
	virtual void endUpdate() = 0;
};

// ---------------------------------------------------------------------------
// Buffer data sources.

class IVertexBufferDataSource {
public:
	virtual unsigned int vertCountForBuffer(CVertexBuffer *vb) = 0;
	virtual CShader *renderShaderForBuffer(CVertexBuffer *vb) = 0;
	virtual unsigned int streamNumberForBuffer(CVertexBuffer *vb) = 0;
	virtual void fillBuffer(CVertexBuffer *vb) = 0;
	virtual bool isSoftwareVertexBuffer(CVertexBuffer *) { return false; }
	virtual ~IVertexBufferDataSource() {}
};

class IIndexBufferDataSource {
public:
	virtual unsigned int indsCountForBuffer(CIndexBuffer *ib) = 0;
	virtual CShader *renderShaderForBuffer(CIndexBuffer *ib) = 0;
	virtual bool use32BitsForBuffer(CIndexBuffer *ib) = 0;
	virtual void fillBuffer(CIndexBuffer *ib) = 0;
	virtual ~IIndexBufferDataSource() {}
};

class IBuffersOwnerDataSource {
public:
	virtual D3DPRIMITIVETYPE primitiveTypeForBufOwner(CBuffersOwner *owner) = 0;
	virtual CVertexBuffer *vertexBufferForBufOwner(int stream, CBuffersOwner *owner) = 0;
	virtual CIndexBuffer *indexBufferForBufOwner(CBuffersOwner *owner) = 0;
	virtual ~IBuffersOwnerDataSource() {}
};

// ---------------------------------------------------------------------------
// Device buffers filled by a data source.

class CVertexBuffer : public CContextObject {
	LF_DECLARE_CLASS(CVertexBuffer)
public:
	CVertexBuffer();
	virtual void dealloc();
	virtual void instantiateSelf();
	virtual void releaseSelf();
	// A source that is an LObject is retained when `retainSource` is set.
	virtual CVertexBuffer *initWithDataSource(IVertexBufferDataSource *source, CHolder *holder,
	                                          unsigned char retainSource, unsigned char flag);

	CVertexBuffer *initWithDeviceBuffer(VertexBufferEmu *vb, unsigned int stride);
	void dataSourceWillDie();
	StVertexBufferLocker *detachLocker(unsigned int flags);
	VertexBufferEmu *deviceBuffer() const { return _vb; }
	unsigned int stride() const { return _stride; }
	unsigned int verticesCount() const { return _count; }

	VertexBufferEmu *_vb;
	LObject *_dsOwner;
	IVertexBufferDataSource *_ds;
	unsigned int _stride;
	unsigned int _count;
	bool _flag;
};

class CIndexBuffer : public CContextObject {
	LF_DECLARE_CLASS(CIndexBuffer)
public:
	CIndexBuffer();
	virtual void dealloc();
	virtual void instantiateSelf();
	virtual void releaseSelf();
	virtual CIndexBuffer *initWithDataSource(IIndexBufferDataSource *source, D3DPRIMITIVETYPE type,
	                                         CHolder *holder, unsigned char retainSource, unsigned char flag);

	CIndexBuffer *initWithDeviceBuffer(IndexBufferEmu *ib, D3DPRIMITIVETYPE type);
	void dataSourceWillDie();
	StIndexBufferLocker *detachLocker(unsigned int flags);
	IndexBufferEmu *deviceBuffer() const { return _ib; }
	unsigned int primitivesCount() const { return _primCount; }

	IndexBufferEmu *_ib;
	LObject *_dsOwner;
	IIndexBufferDataSource *_ds;
	D3DPRIMITIVETYPE _primType;
	unsigned int _primCount;
	bool _is32bit;
	bool _flag;
};

// ---------------------------------------------------------------------------
// CDrawableModel: something drawable; each place it is drawn has its own
// CDrawableModelData.

class CDrawableModel : public CContextObject,
                       public CConstDataProvider,
                       public CInstanceConstDataProvider,
                       public CDeviceTextureProvider,
                       public CShaderStageStateProvider,
                       public CClipPlanesProvider {
	LF_DECLARE_ABSTRACT_CLASS(CDrawableModel)
public:
	CDrawableModel(TCtxObjPriority priority = 0);
	// A new autoreleased data of modelDataClass() for this model.
	virtual CDrawableModelData *detachModelDataInstance();
	virtual LClass *modelDataClass() = 0;
	virtual CRenderProfile *renderProfile() = 0;
	virtual bool hasRenderProfile() { return true; }
	// Whether the data is visible; fills its center and distance when asked.
	virtual bool evalVisibilityParams(CDrawableModelData *data, Vector3 *center, float *distance) = 0;
	// 1 opaque, 2 transparent, 3 glow.
	virtual int evaluateDrawableType();
	virtual void doImmediateDrawWithData(CDrawableModelData *data, CFrameDrawer *drawer) = 0;
	virtual void doImmediateDrawWithDataArray(TLArray<CDrawableModelData *> *, CFrameDrawer *) {}
	virtual bool canBeDrawnWithDataArray() { return false; }
	virtual const void *_dataPtrForConstant(const SShaderConstDescr *, Matrix *) { return 0; }
	virtual CConstDataProvider *_nextConstDataProvider() { return 0; }
	virtual const void *_instanceDataPtrForConstant(unsigned int, void *) { return 0; }
	virtual CInstanceConstDataProvider *_nextInstanceConstDataProvider() { return 0; }
	virtual CDeviceTextureProvider *_nextTextureProvider() { return 0; }
	virtual CShaderStageStateProvider *_nextStageStateProvider() { return 0; }
	virtual CClipPlanesProvider *_nextClipPlanesProvider() { return 0; }

	int drawableType()
	{
		if (_drawableType == 0)
			_drawableType = evaluateDrawableType();
		return _drawableType;
	}
	CDrawableModelData *currentData() const { return _currentData; }

	CDrawableModelData *_currentData;	// while drawing
	int _drawableType;	// evaluateDrawableType's, 0 until asked
};

// The node of TRedBlackTree; color 1 is black.
struct CBaseRBTreeEntry {
	CBaseRBTreeEntry() : _parent(0), _left(0), _right(0), _color(0) {}
	CBaseRBTreeEntry *_parent;
	CBaseRBTreeEntry *_left;
	CBaseRBTreeEntry *_right;
	int _color;
};

class CDrawableModelData : public LObject,
                           public CBaseRBTreeEntry,
                           public CSceneStatePropertyOwner,
                           public CConstDataProvider,
                           public CInstanceConstDataProvider,
                           public CDeviceTextureProvider,
                           public CShaderStageStateProvider,
                           public CClipPlanesProvider {
	LF_DECLARE_CLASS(CDrawableModelData)
public:
	CDrawableModelData();
	virtual void dealloc();
	virtual const Matrix *worldMatrix();
	virtual CDrawableModelData *initWithHostModel(CDrawableModel *model);
	virtual void immediateDraw(CFrameDrawer *drawer);
	virtual void storeSceneStateInStream(LData *, unsigned int) {}
	virtual const void *restoreSceneStateFromData(const void *data, unsigned int) { return data; }
	virtual void renderSelfHolderInScene(CScene *scene, unsigned int order);
	virtual const void *_dataPtrForConstant(const SShaderConstDescr *, Matrix *) { return 0; }
	virtual CConstDataProvider *_nextConstDataProvider() { return _model; }
	virtual const void *_instanceDataPtrForConstant(unsigned int, void *) { return 0; }
	virtual CInstanceConstDataProvider *_nextInstanceConstDataProvider() { return _model; }
	virtual bool _getTextureForSource(BaseTextureEmu **texture, int source);
	virtual CDeviceTextureProvider *_nextTextureProvider() { return _model; }
	virtual CShaderStageStateProvider *_nextStageStateProvider() { return _model; }
	virtual CClipPlanesProvider *_nextClipPlanesProvider() { return _model; }

	bool applyMaterialForSubset(long subset, CShaderStage *stage);
	void releaseWithModel();
	void retainWithModel();
	CDrawableModel *model() const { return _model; }
	CRenderProfile *profile() const { return _profile; }
	const Vector3 *center() const { return &_center; }
	float distance() const { return _distance; }

	Vector3 _center;
	float _distance;
	CDrawableModel *_model;	// retained
	CRenderProfile *_profile;
};

class ModelDataWithMatrix : public CDrawableModelData {
	LF_DECLARE_CLASS(ModelDataWithMatrix)
public:
	ModelDataWithMatrix();
	virtual const Matrix *worldMatrix() { return &_matrix; }
	virtual void storeSceneStateInStream(LData *stream, unsigned int flags);
	virtual const void *restoreSceneStateFromData(const void *data, unsigned int flags);
	void setWorldMatrix(const Matrix *m) { _matrix = *m; }

	Matrix _matrix;
};

// ---------------------------------------------------------------------------
// CBuffersOwner: a model drawn from vertex and index buffers.

class CBuffersOwner : public CDrawableModel {
	LF_DECLARE_ABSTRACT_CLASS(CBuffersOwner)
public:
	CBuffersOwner();
	virtual void dealloc();
	virtual void instantiateSelf() {}
	virtual void releaseSelf() {}
	virtual CRenderProfile *renderProfile() { return _profile; }
	virtual void doImmediateDrawWithData(CDrawableModelData *data, CFrameDrawer *drawer);
	virtual void setRenderProfile(CRenderProfile *profile);
	virtual void invalidateBuffers();
	virtual StVertexBufferLocker *detachVBLocker(int stream, unsigned int flags);
	virtual StIndexBufferLocker *detachIBLocker(unsigned int flags);
	virtual void setVertexBufferForStream(CVertexBuffer *vb, int stream);
	virtual void setIndexBuffer(CIndexBuffer *ib);
	virtual void doImmediateDrawMultiInstance(CParticleSystemData *data, CFrameDrawer *drawer);
	// Returns the number of draw stages.
	virtual long beginDrawing(CDrawableModelData *data);
	virtual void bindBuffers();
	virtual bool activateStage(long stage);
	virtual void drawSubset(CDrawableModelData *data, long subset, long index);
	virtual void drawSubsetMultiInstance(CParticleSystemData *data, long subset, long stage);
	virtual void simpleDraw(unsigned int primCount);
	virtual void endDrawing();

	CBuffersOwner *initWithDataSource(IBuffersOwnerDataSource *source, CRenderProfile *profile,
	                                  unsigned char retainSource);
	void dataSourceWillDie();
	CShader *shader() const { return _shader; }

	static unsigned int numVertsForPrimitive(D3DPRIMITIVETYPE type);

	CRenderProfile *_profile;
	CVertexBuffer *_vbs[kMaxShaderStreams];
	CIndexBuffer *_ib;
	bool _buffersBound;
	unsigned int _streamsCount;
	unsigned int _subsetsCount;
	CShader *_shader;	// the profile's while drawing
	IBuffersOwnerDataSource *_ds;
	LObject *_dsOwner;
	D3DPRIMITIVETYPE _primType;
	unsigned int _primCount;
	unsigned int _numVertices;
};

// ---------------------------------------------------------------------------
// Draw queues: red-black trees of model data ordered by a compare callback.

typedef int (*TDrawEntryCompare)(const CDrawableModelData *a, const CDrawableModelData *b, class CDrawQueue *queue);

struct SDrawEntryCompare {
	static CDrawableModelData *dataForEntry(CBaseRBTreeEntry *e) { return e ? static_cast<CDrawableModelData *>(e) : 0; }
};

template <typename C>
class TRedBlackTree {
public:
	TRedBlackTree(CDrawQueue *queue) : _root(0), _queue(queue) {}
	void insertEntry(CBaseRBTreeEntry *entry);
	void _simpleInsert(CBaseRBTreeEntry *entry);
	void _rotateLeft(CBaseRBTreeEntry *x);
	void _rotateRight(CBaseRBTreeEntry *x);

	CBaseRBTreeEntry *_root;
	CDrawQueue *_queue;
};

// In-order traversal.
template <typename C>
class TRBTreeIterator {
public:
	TRBTreeIterator(TRedBlackTree<C> *tree) : _tree(tree), _current(0), _atEnd(false) {}
	bool next();
	CDrawableModelData *data() const { return C::dataForEntry(_current); }

	TRedBlackTree<C> *_tree;
	CBaseRBTreeEntry *_current;	// 0 before the start
	bool _atEnd;
};

class CDrawQueue {
public:
	CDrawQueue() : _tree(0), _compare(0) {}
	virtual ~CDrawQueue();
	virtual void createQueueArray();
	virtual TDrawEntryCompare _compareCallback() = 0;

	void executeQueue(CFrameDrawer *drawer);
	void clearQueue();
	void postDrawDataToQueue(CDrawableModelData *data);

	TRedBlackTree<SDrawEntryCompare> *_tree;
	TDrawEntryCompare _compare;
};

// By profile name, then nearest first.
class COpaqueObjectsDrawQueue : public CDrawQueue {
public:
	virtual TDrawEntryCompare _compareCallback() { return opaqueObjectCompare; }
	static int opaqueObjectCompare(const CDrawableModelData *a, const CDrawableModelData *b, CDrawQueue *queue);
};

// Farthest first.
class CTranspObjectsDrawQueue : public CDrawQueue {
public:
	virtual TDrawEntryCompare _compareCallback() { return transpObjectCompare; }
	static int transpObjectCompare(const CDrawableModelData *a, const CDrawableModelData *b, CDrawQueue *queue);
};

// By profile name.
class CGlowsDrawQueue : public CDrawQueue {
public:
	virtual TDrawEntryCompare _compareCallback() { return glowsCompare; }
	static int glowsCompare(const CDrawableModelData *a, const CDrawableModelData *b, CDrawQueue *queue);
};

// ---------------------------------------------------------------------------
// CFrameDrawer: collects a frame's model data by order and draws them.

typedef TLIntPtrMap<TLArray<CDrawableModelData *> *> TModelBatchesMap;

class CFrameDrawer : public LObject {
	LF_DECLARE_CLASS(CFrameDrawer)
public:
	CFrameDrawer();
	virtual CFrameDrawer *init() { return this; }
	virtual void dealloc();

	void beginFrame(CScene *scene, CScenePropertiesSet *properties);
	void renderDrawableData(CDrawableModelData *data, TOrder order);
	void flushRenderingQueue();
	void endFrame();
	void _immediateDrawForAllQueues(bool onlyDrawn);
	void _updateEnvironmentMap(CEnvironmentMap *map);
	CContext *context() const { return _context; }
	unsigned int frameNumber() const { return _frameNumber; }
	TArray<CDrawableModelData *> *drawnDatas() const { return _drawnDatas; }

	LSet *_usedModels;
	CHashSet *_drawnDataSet;
	TArray<CDrawableModelData *> *_drawnDatas;
	LSet *_usedProfiles;
	LArray *_orders[5];	// order 0 is sorted into the queues
	TModelBatchesMap *_batches[5];	// models drawn with data arrays; order 0's is unused
	int _capacity;
	CContext *_context;
	CContextCamera *_camera;
	unsigned int _frameNumber;
	CScenePropertiesSet *_properties;
};

#endif
