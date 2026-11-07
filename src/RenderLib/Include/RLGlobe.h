// The globe's earth: the adaptive sphere of texture patches, the terrain
// (ground elements made from the DEM tiles) and their shader stages.

#ifndef RLGLOBE_H
#define RLGLOBE_H

#include "RLScenes.h"

// The earth's shader stages: the day and night switch (and on the globe,
// the ocean transparency) as shader variables.
class EarthGlobeMapShaderStage : public CShaderStage {
	LF_DECLARE_CLASS(EarthGlobeMapShaderStage)
public:
	virtual EarthGlobeMapShaderStage *getOwnerLObject() { return this; }
	virtual const int *addressOfVariable(LString *name);
};

class EarthMapShaderStage : public CShaderStage {
	LF_DECLARE_CLASS(EarthMapShaderStage)
public:
	virtual EarthMapShaderStage *getOwnerLObject() { return this; }
	virtual const int *addressOfVariable(LString *name);
};

// ---------------------------------------------------------------------------
// Vertex buffer fillers of the adaptive meshes. A LOD's buffer is the grid's
// (size + 1)^2 vertices row by row (with the cell centers after each row but
// the last for odd LODs), then the 4 * size border vertices. The template
// flags say which fields are filled: the position, z (the position element),
// the color element and the texture coordinates. The degraded fillers copy
// each field's value at the next coarser LOD into the 4 bytes after it (8 for
// the texture coordinates), for the shader's LOD morph.

// Copies the outer rows and columns to the border vertices, with z (at
// `zOffset`, if not -1) set to `borderZ` (the skirts).
class CBordersFiller {
public:
	static void copyGeometryToBorders(int size, unsigned char centers, char *data, int stride, int zOffset,
	                                  float borderZ);
};

// Fills a LOD row by row: Executor::startFilling(size, centers, context),
// then for each row Executor::startRow(y, center, context) and for each of
// its vertices Executor::fillVertex(x, center, vertex, context).
template <class Executor, class Context>
class TAdaptiveVBCustomFiller {
public:
	void fillAdaptiveVertexBufferCustom(int size, unsigned char centers, char *data, int stride, Context context)
	{
		Executor::startFilling(size, centers, context);
		for (int y = 0; y <= size; y++) {
			Executor::startRow(y, false, context);
			for (int x = 0; x <= size; x++, data += stride)
				Executor::fillVertex(x, false, data, context);
			if (centers && y < size) {
				Executor::startRow(y, true, context);
				for (int x = 0; x < size; x++, data += stride)
					Executor::fillVertex(x, true, data, context);
			}
		}
	}
};

template <unsigned char XY, unsigned char Z, unsigned char Color, unsigned char UV>
class TAdaptiveVBFiller {
public:
	struct SFillerContext {
		float size;
		float halfStep;	// half a cell, in 0..1
		float u[1025];	// of each column
		float uCenter[1025];
		float v;	// of the row
		int xyOffset;
		int zOffset;
		int colorOffset;
		int uvOffset;
	};

	// z 0 and the color an up normal; the texture coordinates span 0..1.
	class FillerExecutor {
	public:
		static void startFilling(int size, int centers, SFillerContext &c);
		static void startRow(int y, bool center, SFillerContext &c)
		{
			c.v = (float)y / c.size;
			if (center)
				c.v += c.halfStep;
		}
		static void fillVertex(int x, bool center, char *vertex, SFillerContext &c)
		{
			if (Z)
				*(float *)(vertex + c.zOffset) = 0.0f;
			if (Color) {
				unsigned char *color = (unsigned char *)(vertex + c.colorOffset);
				color[0] = 0x80;
				color[1] = 0x80;
				color[2] = 0xff;
				color[3] = 0x80;
			}
			// The original stored the cell centers' coordinates at 4 * uvOffset
			// (a float index); the same while the coordinates come first.
			if (UV) {
				float *uv = (float *)(vertex + c.uvOffset);
				uv[0] = center ? c.uCenter[x] : c.u[x];
				uv[1] = c.v;
			}
		}
	};

	// Fills `lod` and its borders (z `borderZ` where filled).
	static void fillVertices_LOD(char *data, long stride, int lod, int xyOffset, int zOffset, int colorOffset,
	                             int uvOffset, float borderZ);
};

template <unsigned char XY, unsigned char Z, unsigned char Color, unsigned char UV>
class TAdaptiveVBDegradedFiller {
public:
	// The field pointers are at the first vertex's fields (0 if not filled).
	static void fillVertices_LOD_2n(int lod, long stride, char *xy, char *z, char *color, char *uv);
	static void fillVertices_LOD_2n_plus_1(int lod, long stride, char *xy, char *z, char *color, char *uv);
};

// The patch bounds of a LOD's vertices, in IAdaptiveMeshDataSource order.
struct SBoundCalcParams {
	int stride;
	int xyOffset;
	int zOffset;
	Vector2 *mins;
	Vector2 *maxs;
	float *minZ;
	float *maxZ;
	bool centers;
	int size;
	int rowStride;	// vertices
	int lod;
};

template <unsigned char XY, unsigned char Z>
class TAdaptiveVBBoundsUpdater {
public:
	// Patch (x, y) of `level` at `data`; returns its bounds in min and max.
	static void updateBounds(SBoundCalcParams *params, char *data, long level, long x, long y, Vector3 *min,
	                         Vector3 *max);
};

// The bounds of every patch: xy into mins/maxs, z into minZ/maxZ (either
// pair 0 to skip it).
void updateBoundsWhole(int lod, char *data, int stride, int xyOffset, int zOffset, Vector2 *mins, Vector2 *maxs,
                       float *minZ, float *maxZ);

// ---------------------------------------------------------------------------
// The globe: the sphere's patches down to level 4 drawn from one mesh with
// the textures of the level their distance needs; nearer, level 5 patches
// drawn as ground elements.

class AdaptiveSphereDrawableData;

class AdaptiveLODSphere : public LObject, public CSceneStatePropertyOwner {
	LF_DECLARE_CLASS(AdaptiveLODSphere)
public:
	// A patch to draw and where its texture and city lights are.
	struct SPatchDrawInfo {
		SPatchDrawInfo(const STextureID &id, unsigned char local);
		// Takes the textures loaded, up to `maxLevel`.
		void adjustWithLoader(unsigned int maxLevel, CTextureLoader *loader);

		STextureID id;
		bool local;	// texture coordinates within the patch
		STextureUnitTransforms texture;
		STextureUnitTransforms glows;
	};

	// The frame's view, for the patches.
	struct SDrawPartParams {
		const Matrix *viewMatrix;
		float farClip;
		float pixelScale;	// 600 / the frame's height
		const Vector3 *cameraPosition;
		float cameraDistance2;
		CContext *context;
		CFrameDrawer *drawer;
		SphereLOD::SCameraParams *camera;
	};

	AdaptiveLODSphere();
	virtual void dealloc();
	virtual AdaptiveLODSphere *getOwnerLObject() { return this; }
	virtual void storeSceneStateInStream(LData *stream, unsigned int flags);
	virtual const void *restoreSceneStateFromData(const void *data, unsigned int flags);
	virtual void renderSelfHolderInScene(CScene *scene, unsigned int flags);
	virtual intptr_t _waterTxtResWasChanged(char *args);

	AdaptiveLODSphere *initWithParams(CHolder *holder);
	// `visibility` fades the globe in (it is transparent below 0.4).
	void performDraw(CFrameDrawer *drawer, float visibility);
	AdaptiveSphereDrawableData *wholeGlobeData();
	void setNightGlowsVisibility(float visibility);
	void setAnimatedWaterVisibility(float visibility);
	void setAnimatedWaterSpecularsVisibility(float visibility);
	void setAnimatedWaterDayFactor(float factor);

	// The fog's far distance for a far clip distance.
	static float calcFogClip(float farClip);
	// The texture level for a patch at `distance`, seen at `cosAngle`.
	int _calcTextureLevel(float distance, float cosAngle, float pixelScale);
	// Draws the patch, or its children where nearer parts need finer
	// textures; the levels are those of its far and near ends (-1: unknown).
	void drawPart(const STextureID &id, CFrameDrawer *drawer, const SDrawPartParams *params, float nearDistance,
	              float farDistance, int farLevel, int nearLevel);
	void _drawElementWithTextureLevel(const STextureID &id, int level, const SDrawPartParams *params);
	// Shows water frame `frame` (by the time if -1).
	void _updateWaterTexture(int frame);

	bool _waterVisible;
	LArray *_waterFrames;	// LData of the DDS files read, LNull until then
	C2DTexture *_waterTexture;
	bool _lowResWater;
	int _waterFrame;
	float _nightGlowsVisibility;
	float _waterVisibility;
	float _waterSpecularsVisibility;
	float _waterDayFactor;
	CHolder *_holder;
	float _lodScale;
	float _unused4C;
	AdaptiveSphereDrawableData *_data;
	bool _hasPatches;	// this frame
	bool _replaying;
	bool _transparent;	// the far side shows: draw everything
	float _visibility;
};

class AdaptiveSphereDrawable : public CMesh, public CMeshDataSource {
	LF_DECLARE_CLASS(AdaptiveSphereDrawable)
public:
	AdaptiveSphereDrawable() : _textureStage(-1), _glowsStage(-1), _textureConst(-1), _glowsConst(-1) {}
	virtual void dealloc();
	virtual AdaptiveSphereDrawable *getOwnerLObject() { return this; }
	virtual LClass *modelDataClass();
	virtual bool _getTextureForSource(BaseTextureEmu **texture, int source);
	virtual const void *_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer);
	virtual bool activateStage(long stage);
	// Draws each patch with its textures.
	virtual void drawSubset(CDrawableModelData *data, long subset, long index);
	virtual D3DPRIMITIVETYPE primitiveTypeForMesh(CMesh *) { return D3DPT_TRIANGLELIST; }
	virtual unsigned int numPrimitivesForMesh(CMesh *) { return 11664; }
	virtual unsigned int numVerticesForMesh(CMesh *) { return 5995; }
	// A 109 x 55 grid over the sphere; each patch's triangles are together.
	virtual void meshWasInstantiated(CMesh *mesh);

	template <typename T> void _fillIndexBuffersForAdaptiveSphere(T *indices, int x, int y, int level);

	int _textureStage;
	int _glowsStage;
	int _textureConst;
	int _glowsConst;
};

class AdaptiveSphereDrawableData : public ModelDataWithMatrix {
	LF_DECLARE_CLASS(AdaptiveSphereDrawableData)
public:
	AdaptiveSphereDrawableData();
	virtual AdaptiveSphereDrawableData *getOwnerLObject() { return this; }
	// 'FPGl': the fog; 'FPGm': the transparency.
	virtual const void *_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer);
	// Stage 2 (the back faces) only while transparent.
	virtual bool _isStageOn(CShaderStage *stage, unsigned char *on);

	void startFrame(float farClip, float visibility);
	void addElement(const STextureID &id, int textureLevel);
	static int _comparePatchInfoByTexture(const void *a, const void *b, const void *context);

	TArray<AdaptiveLODSphere::SPatchDrawInfo> _patches;	// sorted by texture
	float _fogClip;
	float _alpha;
};

// ---------------------------------------------------------------------------
// An adaptive mesh over a sphere patch: the flat grid bent into the patch's
// trapezoid by the shader ('GETr') and placed by the level 5 matrices.

class GlobeAdaptiveMesh : public CAdaptiveFlatMesh {
	LF_DECLARE_CLASS(GlobeAdaptiveMesh)
public:
	virtual GlobeAdaptiveMesh *getOwnerLObject() { return this; }
	virtual LClass *modelDataClass();
	virtual bool evalVisibilityParams(CDrawableModelData *data, Vector3 *center, float *distance);
};

class GlobeAdaptiveMeshData : public ModelDataWithMatrix {
	LF_DECLARE_CLASS(GlobeAdaptiveMeshData)
public:
	GlobeAdaptiveMeshData();
	virtual void dealloc();
	virtual GlobeAdaptiveMeshData *getOwnerLObject() { return this; }
	// 'GETr' the trapezoid, 'FPGl' the fog, 'TTrn'/'TTro' the texture and
	// city lights transforms.
	virtual const void *_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer);
	// Source 0 the patch texture, 1 the city lights.
	virtual bool _getTextureForSource(BaseTextureEmu **texture, int source);

	// The mesh's TAdaptiveMeshTransformCallback: a flat box to the trapezoid's.
	static void transformBoundingBox(Vector3 *min, Vector3 *max, CDrawableModelData *data);
	// For patch `id` (level 5 for the matrices) with the ground at `zOffset`.
	void setTransforms(const STextureID &id, float zOffset);
	void setVerticalScaleFactor(float factor);

	AdaptiveLODSphere::SPatchDrawInfo _drawInfo;
	STextureID _id;
	Vector2 _trapezoid;	// SphereLOD::entryTrapetionParams
	float _fogClip;
	float _zOffset;
};

// ---------------------------------------------------------------------------
// The terrain: a level 5 patch near the camera is a GroundElement, an
// adaptive mesh (LODs 4-14, a 128 x 128 grid at most) whose heights and
// normals come from its DEM file, loaded on a task queue when the mesh needs
// LOD 8 or more. The elements of a holder are cached by GroundElementsManager.

class GlobeAdaptiveMeshData;
class GroundElementModel;
class GroundElementsManagerData;

class GroundElement : public LObject, public IVertexBufferDataSource, public IAdaptiveMeshDataSource {
	LF_DECLARE_CLASS(GroundElement)
public:
	GroundElement();
	virtual void dealloc();
	virtual GroundElement *getOwnerLObject() { return this; }
	// The height and normal buffers, stream 1 (LODs 0-14).
	virtual unsigned int vertCountForBuffer(CVertexBuffer *vb);
	virtual CShader *renderShaderForBuffer(CVertexBuffer *vb);
	virtual unsigned int streamNumberForBuffer(CVertexBuffer *vb);
	virtual void fillBuffer(CVertexBuffer *vb);
	virtual char logGridFullSize();
	virtual CRenderProfile *renderProfile();
	// Stream 0 is the manager's shared grid; stream 1 the heights, or the
	// shared flat ones without the DEM or the relief.
	virtual CVertexBuffer *vertexBufferForLOD(int stream, int lod);
	virtual void getMinMaxCorners(Vector2 **mins, Vector2 **maxs, float **minZ, float **maxZ);
	virtual CSharableResource *boundsMutex();
	virtual intptr_t _demLoadComplete(char *args);
	virtual intptr_t _mainThreadReleaser(char *args);

	GroundElement *initWithTextureID(const STextureID &id, CHolder *holder);
	bool requiresWaterRendering();
	bool demDataIsAvailable() { return _demAvailable; }
	void setLastRenderTime(unsigned int time) { _lastRenderTime = time; }
	void releaseMemoryResources();
	CVertexBuffer *heightNormalsVertexBufferForLOD(int lod) { return _heightVBs[lod]; }
	// The patch's mesh data with the textures up to `textureLevel` and the
	// fog of `farClip`; 0 if it can't be made.
	GlobeAdaptiveMeshData *tryToRender(int textureLevel, float farClip);
	void _setLODBias(float bias);
	int _lodFromBuffer(CVertexBuffer *vb);
	void _releaseHeightVBs();
	GlobeAdaptiveMeshData *_drawableData();

	static float borderZValue();
	static void finalize();
	static void setNightGlowsVisibility(float visibility);
	static void setAnimatedWaterVisibility(float visibility);
	static void setAnimatedWaterSpecularsVisibility(float visibility);
	static void setAnimatedWaterDayFactor(float factor);
	static void setWaterTextureScale(float scale);
	static void setAnimatedWaterLightColor(const Color *color);
	static void setLODBias(float bias);
	static float LODBias();
	// The patch's lowest and highest ground, in globe radii above the sphere;
	// false without its element.
	static bool getZRange(const STextureID &id, float *minZ, float *maxZ);
	// Queues the patch to be drawn with textures up to `textureLevel`.
	static void postElementToQueue(const STextureID &id, int textureLevel, float farClip);
	static void startFrame(CFrameDrawer *drawer, CBaseTexture *waterTexture, LPoint cameraMapPoint);
	static void endFrame(CFrameDrawer *drawer);
	static GroundElement *elementWithID(const STextureID &id, unsigned char create);
	static GroundElementModel *groundMesh(const STextureID &id, unsigned char create);
	// DEMs/<e|w><lon><n|s><lat>/<n>.raw: 81 per 10 degree square; 0 below level 5.
	static LString *heightMapFilePath(const STextureID &id);
	// Reads the element's DEM into heights and normals for LODs 4-14.
	static void loadDEMToMemory(GroundElement *element);

	static LString *kDEMLoadCompleteNotification;	// object: the element

	CHolder *_holder;
	STextureID _id;
	int _unused28;
	bool _demMissing;	// no DEM file (never read)
	bool _unused2D;
	unsigned int _lastRenderTime;
	LArray *_heights;	// per LOD from 4: LData of (z, packed normal) pairs; LNull once in a buffer
	CSharableResource _heightsLock;
	GlobeAdaptiveMeshData *_data;
	CVertexBuffer *_heightVBs[15];
	bool _demDisabled;	// never set
	bool _demLoading;
	bool _demAvailable;
	int _minDrawnLOD;
	int _maxDrawnLOD;
	bool _hasWater;
	bool _computeZBounds;
	float *_minZ;	// of each patch, from the full DEM
	float *_maxZ;
	CSharableResource _boundsLock;
};

// The Z filler's context: the heights (z and the packed normal) of a LOD.
struct SZFillerContext {
	int size;
	float halfStep;
	int rowStride;	// vertices
	int rowBase;	// the row's first height
	int zOffset;
	int colorOffset;
	int degradedColorOffset;	// unused
	const float *heights;	// 0: flat
};

class ZFillerExecutor {
public:
	static void startFilling(int size, int centers, SZFillerContext &c)
	{
		c.size = size;
		c.halfStep = 0.5f / (float)size;
		c.rowStride = size + 1 + (centers ? size : 0);
	}
	static void startRow(int y, bool center, SZFillerContext &c)
	{
		c.rowBase = y * c.rowStride + (center ? c.size + 1 : 0);
	}
	static void fillVertex(int x, bool, char *vertex, SZFillerContext &c)
	{
		const float *h = c.heights ? c.heights + (x + c.rowBase) * 2 : 0;
		*(float *)(vertex + c.zOffset) = h ? h[0] : 0.0f;
		*(float *)(vertex + c.colorOffset) = h ? h[1] : 0.0f;
	}
};

class GroundElementModel : public GlobeAdaptiveMesh {
	LF_DECLARE_CLASS(GroundElementModel)
public:
	virtual void dealloc();
	virtual GroundElementModel *getOwnerLObject() { return this; }
	// 'AnWP' the water's visibility, texture scale and polar fade; 'AnWQ'
	// to 'AnWT' and 'TTrp' the water and city light colors.
	virtual const void *_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer);
	// Subset 0 the water's alpha, 1 its light color, as the texture factor.
	virtual void drawSubset(CDrawableModelData *data, long subset, long index);
	virtual void changeCurrentLoadedLODTo(int lod);
	// While large views are grabbed: loads the DEM now.
	virtual bool preloadLODBeforeDraw(int lod);
	// Queues the DEM's load once LOD 8 is drawn.
	virtual void analyzeLODRangeAfterDraw(int minLOD, int maxLOD);
	virtual intptr_t _reliefStateChanged(char *args);
	// The day and night, water and no-water shader stages.
	virtual bool _isStageOn(CShaderStage *stage, unsigned char *on);

	GroundElementModel *initWithGroundElement(GroundElement *element, CRenderProfile *profile);

	GroundElement *_element;
};

class GroundElementsManager : public LObject, public IVertexBufferDataSource {
	LF_DECLARE_CLASS(GroundElementsManager)
public:
	GroundElementsManager();
	virtual void dealloc();
	virtual GroundElementsManager *getOwnerLObject() { return this; }
	virtual intptr_t _contextWillBeDestroyed(char *args);
	virtual intptr_t _lodWasChanged(char *args);
	virtual intptr_t _reliefStateChanged(char *args);
	// The shared buffers: stream 0 the grid's texture coordinates (and the
	// xy bounds), stream 1 flat heights with up normals.
	virtual unsigned int vertCountForBuffer(CVertexBuffer *vb);
	virtual CShader *renderShaderForBuffer(CVertexBuffer *vb);
	virtual unsigned int streamNumberForBuffer(CVertexBuffer *vb);
	virtual void fillBuffer(CVertexBuffer *vb);

	void setLODBias(float bias);
	CVertexBuffer *sharedEmptyZNrmBufferForLOD(int lod);
	CVertexBuffer *sharedXYTBufferForLOD(int lod);
	CRenderProfile *sharedRenderProfile();
	CBaseTexture *darkWaterColorTexture();
	LTaskQueue *loadingQueue();
	GroundElement *elementWithID(const STextureID &id, unsigned char create);
	GroundElementModel *groundMesh(const STextureID &id, unsigned char create);
	void postElementToQueue(const STextureID &id, int textureLevel, float farClip);
	// Takes the drawer's holder; the queue is sorted by the distance from
	// `cameraMapPoint` (0..1 longitude and latitude).
	void startFrame(CFrameDrawer *drawer, CBaseTexture *waterTexture, LPoint cameraMapPoint);
	// Draws the frame's elements; cancels loads of elements not drawn.
	void endFrame(CFrameDrawer *drawer);
	void markElementAsDEMLoading(GroundElement *element);
	void markElementAsDEMLoaded(GroundElement *element);
	GroundElement *_createElement(const STextureID &id);
	GroundElementsManagerData *_specificDataForHolder(CHolder *holder);
	void _releaseOldestElements(int count);
	// With the xy bounds of every patch if `bounds` (LOD 14).
	void _fillSharedXYBufferForLOD(int lod, unsigned char bounds);

	static int _compareGroundElementsByTime(const void *a, const void *b, const void *context);
	static void setAnimatedWaterDayFactor(float factor);

	static bool mDrawWithRelief;
	static float mAnimatedWaterVisibility;
	static bool mAnimatedWaterIsEnabled;
	static float mAnimatedWaterSpecsVisibility;
	static float mWaterTextureScale;
	static Color mNightCityGlowsColor;
	static Color mLightWaterConstColor;
	static Color mDarkWaterColor;
	static Color mLightWaterColor;
	static Color mLightDarkDiffColor;
	static Color mWaterSpecularColor;

	CContext *_context;
	CHolder *_holder;
	bool _lodBiasApplied;
	GroundElementsManagerData *_data;	// of _holder
	float _lodBias;
	unsigned int _minCachedElements;
	LData *_waterParts;	// 324 x 162 floats: the water in each level 5 patch
};

// A holder's elements and shared buffers.
class GroundElementsManagerData : public LObject {
	LF_DECLARE_CLASS(GroundElementsManagerData)
public:
	GroundElementsManagerData();
	virtual void dealloc();
	virtual GroundElementsManagerData *getOwnerLObject() { return this; }

	static int _compareGEDatasByTexture(const void *a, const void *b, const void *context);

	int _postedCount;	// this frame
	CRenderProfile *_profile;
	CBaseTexture *_waterTexture;	// this frame's
	CBaseTexture *_darkWaterTexture;
	CVertexBuffer *_xyBuffers[15];
	CVertexBuffer *_zNrmBuffers[15];
	Vector2 *_mins;	// xy bounds of the LOD 14 patches
	Vector2 *_maxs;
	CHashTable _loading;	// key: frames waited
	LTaskQueue *_queue;
	unsigned int _cameraKey;	// the queue comparator's context
	TLIntPtrMap<GroundElement *> _elements;
	CHashSet *_drawn;	// this frame's keys
	TArray<GlobeAdaptiveMeshData *> _toDraw;	// sorted by texture
};

class LoadDEMTask : public LTask {
	LF_DECLARE_CLASS(LoadDEMTask)
public:
	LoadDEMTask() : _element(0) {}
	virtual void dealloc();
	virtual LoadDEMTask *getOwnerLObject() { return this; }
	virtual void execute();

	// The task ID is the element's key.
	LoadDEMTask *initWithElement(GroundElement *element);
	// Lower levels first, then the nearer to the camera's patch (`context`).
	static int compare(const void *a, const void *b, const void *context);

	GroundElement *_element;
};

#endif
