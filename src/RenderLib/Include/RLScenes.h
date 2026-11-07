// Scenes: BaseScene, what every panel draws (overlays, devices, captions,
// clouds, grid, points and routes), its globe (PerspectiveScene), flat map
// and logo kinds, and SharedContextObjectsContainer, the textures, fonts,
// buttons and models a view's scenes share.

#ifndef RLSCENES_H
#define RLSCENES_H

#include "AppModel.h"
#include "FHMeshes.h"
#include "FHParticles.h"
#include "FHSprites.h"
#include "FHScene.h"
#include "FH2DContainer.h"
#include "RLTextures.h"

class AtlasPanelController;
class BaseScene;
class CinematicSequence;
class RenderOptions;
class NatureParams;
class Stars;
class Atmosphere;
class CEclipseMesh;
class CEclipseMeshData;
class _EclipseHemisphereDataSource;
class AdaptiveLODSphere;
class GridLineMesh;
class CGridMarkers;
class CCloudsMesh;
class CCloudsMeshData;
class CAllCitiesMeshData;
class CAllPointsContainer;
class SceneState;
class ShapeContainerStateHolder;
class ShapesCaptionStateHolder;
class CRoutePointsContainer;
class CTextureLoader;
class CloudsController;
class EntriesBillboardsHolder;
class PanelDrawIterator;
class BaseDrawing;
class ImagesPreviewController;
class MarkerMesh;
class MarkerModelData;
class AboutMesh;
class AboutMeshData;
class IWindowInterface;
class SharedContextObjectsContainer;

class _ImagesPreviewControllerDS;

// What part of an entry billboard was hit (EntryBillboard::kindFromViewPoint).
enum TBillboardKind { kBillboardNone = 0, kBillboardName = 1, kBillboardWeather = 2, kBillboardPhotos = 4 };

// A route point as the route lines are built from it: on the earth.
struct SRouteEarthPoint {
	Vector3 earthVec;
	Color color;
};

// A piece of the flight along a route: a Hermite curve between two earth
// points with their tangents.
struct SFlySegment {
	Vector3 from;
	Vector3 to;
	Vector3 fromTangent;	// normalized
	Vector3 toTangent;	// normalized
	float length;	// km
};

// The scene's current route: its lines along great circles (split at the
// date line), its point markers, its shapes and their captions, and the
// flight along it.
class CRoutePointsContainer : public LObject, public CMeshDataSource, public CSceneStatePropertyOwner {
	LF_DECLARE_CLASS(CRoutePointsContainer)
public:
	CRoutePointsContainer();
	virtual void dealloc();
	virtual CRoutePointsContainer *getOwnerLObject() { return this; }
	virtual intptr_t _layoutWasCalculated(char *) { return 0; }
	virtual intptr_t _updateCaptionLayouts(char *) { return 0; }
	virtual intptr_t _routeObjectWasChanged(char *args);
	virtual void storeSceneStateInStream(LData *stream, unsigned int flags);
	virtual const void *restoreSceneStateFromData(const void *data, unsigned int flags);
	virtual void renderSelfHolderInScene(CScene *scene, unsigned int flags);
	virtual D3DPRIMITIVETYPE primitiveTypeForMesh(CMesh *) { return D3DPT_LINELIST; }
	virtual unsigned int numPrimitivesForMesh(CMesh *mesh);
	virtual unsigned int numVerticesForMesh(CMesh *mesh);
	virtual void meshWasInstantiated(CMesh *mesh);
	// Appends the great circle from `a` to `b` in `steps` steps; false when
	// the lines are full.
	virtual bool _appendRouteBetweenPoints(const SRouteEarthPoint *a, const SRouteEarthPoint *b, int steps);

	CRoutePointsContainer *initWithWindowInterface(IWindowInterface *window);
	void setScene(BaseScene *scene);
	RouteObject *currentRoute() const { return _route; }
	void setCurrentRoute(RouteObject *route);
	// Shows `route` in place of the current one, which comes back with 0.
	void setRouteReplicant(RouteObject *route);
	LArray *shapesToRender();
	void invalCaptionsLayout() { _releaseVisibleCaptionsData(); }
	void renderRoutes(CFrameDrawer *drawer);
	LData *routeLinesInds();
	LData *routeLinesPoints();

	void startFlying();
	void stopFlying() {}
	// Moves the flight on; false at the end of the route.
	bool flyStepWithDeltaTime(double dt);
	LPoint currentFlyMapPoint() const { return _flyMapPoint; }
	Vector2 currentFlyDirection() const { return _flyDirection; }

	void _releaseVisibleCaptionsData();
	void _releaseRouteDependedDatas();
	void _releaseRouteDependedDrawables();
	void _createPointMarkersSystem();
	void _fillRouteLines();
	void _createRouteLinesMesh();
	void _createVisibleCaptionsDatas();
	void _addFlySegment(const Vector3 &from, const Vector3 &fromTangent, const Vector3 &to, const Vector3 &toTangent);
	void _createFlyingPointsArray();

	IWindowInterface *_window;
	BaseScene *_scene;	// not retained
	CRenderProfile *_linesProfile;
	CMesh *_linesMesh;
	LArray *_linesDatas;	// one per map copy
	CParticleSystemData *_markers;
	RouteObject *_route;
	LData *_linePoints;	// SRoutePoints
	LData *_lineIndices;	// pairs of _linePoints indices
	LArray *_captionTitles;
	LData *_captionPoints;	// the titles' geographic points
	LData *_flySegments;	// SFlySegments
	int _flySegment;
	float _flyDistance;	// along the current segment
	LPoint _flyMapPoint;
	Vector2 _flyDirection;
	bool _onLastSegment;
	RouteObject *_savedRoute;	// while a replicant shows
	bool _hasRoute;	// stored in the scene state
};

class PreviewBBMesh;
class PreviewDrawableData;

// The entry's images as a cover flow: the selected one in front, six each
// side turned away, scrolling to a new selection over 0.4 s.
class ImagesPreviewController : public LObject, public CMeshDataSource {
	LF_DECLARE_CLASS(ImagesPreviewController)
public:
	class PreviewDelegate {
	public:
		virtual int imagesCountForPreview(ImagesPreviewController *preview) = 0;
		// Width over height.
		virtual float imageAspectForPreview(int index, ImagesPreviewController *preview) = 0;
		virtual LString *imageTexturePathForPreview(int index, ImagesPreviewController *preview) = 0;
		virtual void previewFocusWasChangedTo(int, ImagesPreviewController *) {}
		virtual void imageWasClicked(int, ImagesPreviewController *) {}
		virtual void imageWasDblClicked(int, ImagesPreviewController *) {}
	};

	ImagesPreviewController();
	virtual void dealloc();
	virtual ImagesPreviewController *getOwnerLObject() { return this; }
	virtual D3DPRIMITIVETYPE primitiveTypeForMesh(CMesh *) { return D3DPT_TRIANGLELIST; }
	virtual unsigned int numPrimitivesForMesh(CMesh *) { return 4; }
	virtual unsigned int numVerticesForMesh(CMesh *) { return 8; }
	virtual void meshWasInstantiated(CMesh *mesh);

	ImagesPreviewController *initWithHolder(CHolder *holder, PreviewDelegate *delegate);
	// 10 without a delegate.
	int imagesCount();
	int currentSelection() const { return _selection; }
	void scrollToImageAtIndex(int index);
	void dropCache();
	void render(CFrameDrawer *drawer, float alpha);
	C2DTexture *_textureAtIndex(int index);
	PreviewDrawableData *_detachBBData();

	PreviewDelegate *_delegate;
	CHolder *_holder;
	PreviewBBMesh *_mesh;
	LArray *_textures;	// LNull::null until loaded
	Matrix _view;
	int _selection;
	float _scrollFrom;
	float _scrollPosition;
	double _scrollStart;	// -1 when not scrolling
};

// The quad an image is drawn on, with its reflection below.
class PreviewBBMesh : public CMesh {
	LF_DECLARE_CLASS(PreviewBBMesh)
public:
	virtual LClass *modelDataClass();
	virtual PreviewBBMesh *getOwnerLObject() { return this; }
};

// 'HoPs': the image's color.
class PreviewDrawableData : public ModelDataWithMatrix {
	LF_DECLARE_CLASS(PreviewDrawableData)
public:
	PreviewDrawableData() : _color(1, 1, 1, 1) {}
	virtual PreviewDrawableData *getOwnerLObject() { return this; }
	virtual const void *_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer);

	Color _color;
};

// The container's delegate: the images of its _imagesEntry.
class _ImagesPreviewControllerDS final : public ImagesPreviewController::PreviewDelegate {
public:
	_ImagesPreviewControllerDS(SharedContextObjectsContainer *owner) : _owner(owner) {}
	virtual int imagesCountForPreview(ImagesPreviewController *preview);
	virtual float imageAspectForPreview(int index, ImagesPreviewController *preview);
	virtual LString *imageTexturePathForPreview(int index, ImagesPreviewController *preview);

	SharedContextObjectsContainer *_owner;
};

// Billboards laid out on the view without overlapping too much: each entry
// is anchored at a geographic point and sized for the kinds on show; the
// callback places them and draws the visible ones.
class SmartBillboardSystem;

class SmartBillboardData : public LObject {
	LF_DECLARE_ABSTRACT_CLASS(SmartBillboardData)
public:
	// Which way the billboard extends from its point: 0 right and down,
	// 1 right and up, 2 left and down, 3 left and up (the offset the
	// callback gets).
	enum TBBOrientation { kOrientation0 = 0, kOrientation1 = 1, kOrientation2 = 2, kOrientation3 = 3 };

	SmartBillboardData() : _orientation(0), _position(LZeroPoint), _showAlways(false), _kind(0), _visible(false),
	                       _rect(LZeroRect) {}
	virtual LSize bbSizeForVisibleMask(unsigned int mask) = 0;
	virtual LComparable *uniqueKey() = 0;

	SmartBillboardData *initWithParams(LPoint position, int orientation, unsigned char showAlways,
	                                   unsigned int kind);
	int orientation() const { return _orientation; }
	LPoint position() const { return _position; }
	bool showAlways() const { return _showAlways; }
	unsigned int kind() const { return _kind; }
	void setKind(unsigned int kind) { _kind = kind; }

	int _orientation;
	LPoint _position;
	bool _showAlways;
	unsigned int _kind;
	bool _visible;
	LRect _rect;	// in the view, while visible
};

class SmartBillboardSystem : public LObject {
	LF_DECLARE_CLASS(SmartBillboardSystem)
public:
	class ISmartBBCallback {
	public:
		virtual void entryBecomesVisible(SmartBillboardData *entry) = 0;
		virtual void entryBecomesInvisible(SmartBillboardData *entry) = 0;
		// Where the entry's point is in the view (`offset` is its size,
		// signed by orientation); false hides it.
		virtual bool geoPointToViewPoint(LPoint *viewPoint, const LPoint *geoPoint, const LSize *offset) = 0;
		virtual void enumWasStarted(SmartBillboardSystem *) {}
		virtual void enumEntryWithRect(SmartBillboardSystem *system, SmartBillboardData *entry, const LRect *rect) = 0;
		virtual void enumWasFinished(SmartBillboardSystem *) {}
	};

	SmartBillboardSystem();
	virtual SmartBillboardSystem *init();
	virtual void dealloc();
	virtual SmartBillboardSystem *getOwnerLObject() { return this; }
	virtual void addEntry(SmartBillboardData *entry);
	virtual void removeAllEntries();
	virtual void removeAllEntriesWithKind(unsigned int kind);
	virtual void invalLayout() { _layoutValid = false; }
	// Lays the entries out in order, hiding the ones that overlap visible
	// ones too much and, once three are up, all but the always-shown ones
	// when they cover more than _maxCoverage of the view. With `notify`,
	// visibility changes are reported 300 ms later (the next enumEntries).
	virtual void _performFastLayout(ISmartBBCallback *callback, unsigned char notify);
	virtual intptr_t _visChangedDelayedSelector(char *args);

	// Releases the system, stopping its timer.
	void invalidate();
	void setViewSize(LSize size)
	{
		_viewSize = size;
		invalLayout();
	}
	void setVisibleKindsMask(unsigned int mask)
	{
		_visibleKinds = mask;
		invalLayout();
	}
	void setAutohideKindsMask(unsigned int mask)
	{
		_autohideKinds = mask;
		invalLayout();
	}
	SmartBillboardData *entryForKey(LString *key);
	// The visible entries, laid out first if needed (unless `skipIfLaidOut`
	// and laid out already), front to back with `forward`.
	void enumEntries(ISmartBBCallback *callback, unsigned char skipIfLaidOut, unsigned char forward,
	                 unsigned char notify);
	SmartBillboardData *findBillboard(ISmartBBCallback *callback, LPoint viewPoint);

	bool _layoutValid;
	LArray *_entries;
	LMap *_entriesByKey;
	float _maxCoverage;
	TArray<unsigned int> _visibleIndices;
	TArray<unsigned int> _shownIndices;	// visible before the last changes
	bool _visibilityChanged;
	LSize _viewSize;
	unsigned int _visibleKinds;
	unsigned int _autohideKinds;
	LTimer *_visTimer;
};

// An entry's billboard: kind 2 its weather (128 wide), 4 its photos (64);
// 0x100 halves it.
class EntryBillboard : public SmartBillboardData {
	LF_DECLARE_CLASS(EntryBillboard)
public:
	EntryBillboard() : _entry(0) {}
	virtual void dealloc();
	virtual LSize bbSizeForVisibleMask(unsigned int mask);
	virtual LComparable *uniqueKey();
	virtual EntryBillboard *getOwnerLObject() { return this; }

	// At a point entry's point, a shape's largest polygon's center or a
	// route's bounds' center.
	EntryBillboard *initWithEntry(GenericEntry *entry, unsigned int kind);
	GenericEntry *entry() const { return _entry; }
	PointEntry *pointEntry();
	// The kinds in `mask` the billboard shows, narrowed to the one under
	// `point`.
	unsigned int kindFromViewPoint(LPoint point, unsigned int mask);

	GenericEntry *_entry;
};

// The weather and photo billboards of the top cities and the entries with
// images, drawn over a scene or into a printout.
class EntriesBillboardsHolder : public LObject,
                                public CSceneStatePropertyOwner,
                                public SmartBillboardSystem::ISmartBBCallback {
	LF_DECLARE_CLASS(EntriesBillboardsHolder)
public:
	EntriesBillboardsHolder();
	virtual void dealloc();
	virtual EntriesBillboardsHolder *getOwnerLObject() { return this; }
	virtual intptr_t _mainDBWasChanged(char *args);
	virtual intptr_t _entryWasChanged(char *args);
	virtual intptr_t _entryInfoWasChanged(char *args);
	virtual void storeSceneStateInStream(LData *stream, unsigned int flags);
	virtual const void *restoreSceneStateFromData(const void *data, unsigned int flags);
	virtual void renderSelfHolderInScene(CScene *scene, unsigned int flags);
	virtual void entryBecomesVisible(SmartBillboardData *entry);
	virtual void entryBecomesInvisible(SmartBillboardData *entry);
	virtual bool geoPointToViewPoint(LPoint *viewPoint, const LPoint *geoPoint, const LSize *offset);
	virtual void enumEntryWithRect(SmartBillboardSystem *system, SmartBillboardData *entry, const LRect *rect);

	EntriesBillboardsHolder *initWithContainer(SharedContextObjectsContainer *container);
	void invalImagesBillboards();
	EntryBillboard *hitTest(LPoint viewPoint, BaseScene *scene);
	void print(BaseDrawing *drawing);
	// Draws the kinds in `kinds` (2 weather, 4 photos).
	void render(BaseScene *scene, CFrameDrawer *drawer, unsigned int kinds);
	void _render(BaseScene *scene, CFrameDrawer *drawer);
	void _createEntriesBillboards();
	void _renderEntry(EntryBillboard *billboard, const LRect *rect);
	void _printEntry(EntryBillboard *billboard, const LRect *rect);

	SharedContextObjectsContainer *_container;	// not retained
	SmartBillboardSystem *_system;
	bool _imagesDirty;
	BaseDrawing *_drawing;	// while printing
	BaseScene *_scene;	// while drawing or hit testing
	PanelDrawIterator *_iterator;
	unsigned int _visibleKinds;
	CFontDrawableData *_namesFont;
	CFontDrawableData *_temperaturesFont;
	CBillboardModelData *_weatherBillboards;
	CBillboardModelData *_weatherIcons;
	CBillboardModelData *_markers;
	CBillboardModelData *_photoIcons;
};

// ---------------------------------------------------------------------------
// The coordinates grid: one line mesh drawn up to eight lines at a time, each
// with its own matrix, plus the coordinates captions.

class GridLineMeshData : public CDrawableModelData {
	LF_DECLARE_CLASS(GridLineMeshData)
public:
	GridLineMeshData();
	virtual GridLineMeshData *getOwnerLObject() { return this; }
	// 'GMMa': the line matrices.
	virtual const void *_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer);

	// Transposed world-view-projection matrices.
	Matrix *matrixForLineAtIndex(unsigned int index) { return &_lineMatrices[index]; }
	void setLinesRenderCount(unsigned int count) { _linesRenderCount = count; }

	Matrix _lineMatrices[8];
	unsigned int _linesRenderCount;
};

// Eight copies of the line; draws as many as its data's count.
class GridLineMesh : public CMesh {
	LF_DECLARE_CLASS(GridLineMesh)
public:
	virtual LClass *modelDataClass();
	virtual bool evalVisibilityParams(CDrawableModelData *data, Vector3 *center, float *distance);
	virtual GridLineMesh *getOwnerLObject() { return this; }
	virtual void drawMeshFragment(unsigned int vertexStart, unsigned int vertexCount, unsigned int faceStart,
	                              unsigned int faceCount);
};

// Gives the shaders LINECAPS_ANTIALIAS.
class CGridShader : public CShader {
	LF_DECLARE_CLASS(CGridShader)
public:
	virtual CGridShader *getOwnerLObject() { return this; }
	virtual const int *addressOfVariable(LString *name);
};

class CGridMarkers : public LObject, public CSceneStatePropertyOwner, public CMeshDataSource {
	LF_DECLARE_ABSTRACT_CLASS(CGridMarkers)
public:
	CGridMarkers();
	virtual void dealloc();
	virtual void render(CFrameDrawer *drawer) = 0;
	// The steps.
	virtual void storeSceneStateInStream(LData *stream, unsigned int flags);
	virtual const void *restoreSceneStateFromData(const void *data, unsigned int flags);
	virtual void renderSelfHolderInScene(CScene *scene, unsigned int flags);
	virtual D3DPRIMITIVETYPE primitiveTypeForMesh(CMesh *) { return D3DPT_LINELIST; }

	CGridMarkers *initWithScene(BaseScene *scene);
	void setGridMesh(GridLineMesh *mesh);
	// In degrees.
	void setGridSteps(float longitude, float latitude);

	BaseScene *_scene;
	GridLineMesh *_gridMesh;
	float _longitudeStep;
	float _latitudeStep;
};

// The flat map's: straight lines, the latitudes captioned at the right, the
// longitudes at the bottom.
class CFlatMapGridMarkers : public CGridMarkers {
	LF_DECLARE_CLASS(CFlatMapGridMarkers)
public:
	virtual void render(CFrameDrawer *drawer);
	virtual CFlatMapGridMarkers *getOwnerLObject() { return this; }
	virtual unsigned int numPrimitivesForMesh(CMesh *) { return 8; }
	virtual unsigned int numVerticesForMesh(CMesh *) { return 0x10; }
	virtual void meshWasInstantiated(CMesh *mesh);
};

// The globe's: unit circles turned into meridians and scaled into parallels.
class CGlobeMapGridMarkers : public CGridMarkers {
	LF_DECLARE_CLASS(CGlobeMapGridMarkers)
public:
	virtual void render(CFrameDrawer *drawer);
	virtual CGlobeMapGridMarkers *getOwnerLObject() { return this; }
	virtual unsigned int numPrimitivesForMesh(CMesh *) { return 0x5a0; }
	virtual unsigned int numVerticesForMesh(CMesh *) { return 0x5a8; }
	virtual void meshWasInstantiated(CMesh *mesh);

	// The latitudes (from, to) and longitudes (from, to, 0..360), in degrees,
	// seen from the camera at `cameraPosition`.
	static void calcRenderParams(const Vector3 *cameraPosition, float *latFrom, float *latTo, float *lonFrom,
	                             float *lonTo);
};

// The flat map's route points: markers sized by the preferences.
class CityMarkerModelData : public CParticleSystemData {
	LF_DECLARE_CLASS(CityMarkerModelData)
public:
	virtual CityMarkerModelData *getOwnerLObject() { return this; }
	// 'prqd': the marker size.
	virtual const void *_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer);
};

class CityMarkerModel : public CParticleSystem {
	LF_DECLARE_CLASS(CityMarkerModel)
public:
	virtual LClass *modelDataClass();
	virtual CityMarkerModel *getOwnerLObject() { return this; }
};

// Reads an .ase plist's first mesh: `*vertices` an LData a vertex, `*faces`
// the index LDatas, joined into one with `joinFaces`; both retained, the
// big-endian words swapped.
bool parseASEFile(LString *path, unsigned char unused, unsigned char joinFaces, LArray **vertices, LArray **faces);

// The globe's route points: pushpins, a model drawn once a particle.
class CPushpinDataSource : public LObject,
                           public IBuffersOwnerDataSource,
                           public IIndexBufferDataSource,
                           public IVertexBufferDataSource {
	LF_DECLARE_CLASS(CPushpinDataSource)
public:
	CPushpinDataSource();
	virtual void dealloc();
	virtual CPushpinDataSource *getOwnerLObject() { return this; }
	virtual D3DPRIMITIVETYPE primitiveTypeForBufOwner(CBuffersOwner *) { return D3DPT_TRIANGLELIST; }
	virtual CVertexBuffer *vertexBufferForBufOwner(int stream, CBuffersOwner *owner);
	virtual CIndexBuffer *indexBufferForBufOwner(CBuffersOwner *owner);
	virtual unsigned int indsCountForBuffer(CIndexBuffer *ib);
	virtual CShader *renderShaderForBuffer(CIndexBuffer *ib);
	virtual bool use32BitsForBuffer(CIndexBuffer *ib);
	virtual void fillBuffer(CIndexBuffer *ib);
	virtual unsigned int vertCountForBuffer(CVertexBuffer *vb);
	virtual CShader *renderShaderForBuffer(CVertexBuffer *vb);
	virtual unsigned int streamNumberForBuffer(CVertexBuffer *) { return 0; }
	virtual void fillBuffer(CVertexBuffer *vb);

	CPushpinDataSource *initWithASEFileAndRenderProfile(LString *path, CRenderProfile *profile);

	LArray *_faces;
	LArray *_vertices;
	CVertexBuffer *_vb;
	CIndexBuffer *_ib;
	CRenderProfile *_profile;
};

class CPushpinModel : public CBuffersOwner {
	LF_DECLARE_CLASS(CPushpinModel)
public:
	virtual LClass *modelDataClass();
	virtual bool evalVisibilityParams(CDrawableModelData *, Vector3 *, float *) { return true; }
	virtual CPushpinModel *getOwnerLObject() { return this; }
};

class CPushpinParticleSystem : public CParticleSystem {
	LF_DECLARE_CLASS(CPushpinParticleSystem)
public:
	CPushpinParticleSystem() : _jumpingMarkers(false) {}
	virtual LClass *modelDataClass();
	virtual void doImmediateDrawWithData(CDrawableModelData *data, CFrameDrawer *drawer);
	virtual CPushpinParticleSystem *getOwnerLObject() { return this; }

	void enableJumpingMarkers(unsigned char enable) { _jumpingMarkers = enable; }

	bool _jumpingMarkers;
};

class CPushpinParticleSystemData : public CParticleSystemData {
	LF_DECLARE_CLASS(CPushpinParticleSystemData)
public:
	CPushpinParticleSystemData() : _transforms(0), _scale(1.0f), _fogVisibility(0.0f) {}
	virtual void dealloc();
	virtual CPushpinParticleSystemData *getOwnerLObject() { return this; }
	virtual void storeSceneStateInStream(LData *stream, unsigned int flags);
	virtual const void *restoreSceneStateFromData(const void *data, unsigned int flags);
	// 'pppm': the scale and the fog.
	virtual const void *_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer);
	// A pushpin's rotation, position and colour.
	virtual const void *_instanceDataPtrForConstant(unsigned int index, void *buffer);

	void setScale(float scale) { _scale = scale; }
	void setFogVisibility(float visibility) { _fogVisibility = visibility; }
	void invalidate() { _instancingDirty = true; }
	// The pushpins' rotations, recomputed when the particles changed.
	void updateTransformsData();
	float _pushpinAnimationFactor(int index, double time);

	LData *_transforms;	// a 3 x 3 matrix a particle
	float _scale;
	float _fogVisibility;
};

// The about box's logo: a mesh drawn over the globe, 'pcom' its time.
class AboutMesh : public CMesh {
	LF_DECLARE_CLASS(AboutMesh)
public:
	AboutMesh() { setPerformBoxVisibilityTest(0); }
	virtual LClass *modelDataClass();
	virtual AboutMesh *getOwnerLObject() { return this; }
};

class AboutMeshData : public ModelDataWithMatrix {
	LF_DECLARE_CLASS(AboutMeshData)
public:
	AboutMeshData() : _time(0.0f) {}
	virtual AboutMeshData *getOwnerLObject() { return this; }
	virtual const void *_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer);

	float _time;
};

// The marker drawn at the hit city: a textured quad in a color ('pcol'),
// with constant 'pcon' (1, 0, 0, 0).
class MarkerMesh : public CMesh {
	LF_DECLARE_CLASS(MarkerMesh)
public:
	virtual LClass *modelDataClass();
	virtual MarkerMesh *getOwnerLObject() { return this; }
};

class MarkerModelData : public ModelDataWithMatrix {
	LF_DECLARE_CLASS(MarkerModelData)
public:
	MarkerModelData() : _color(1, 1, 1, 1), _texture(0), _params(1, 0, 0, 0) {}
	virtual void dealloc();
	virtual MarkerModelData *getOwnerLObject() { return this; }
	virtual void storeSceneStateInStream(LData *stream, unsigned int flags);
	virtual const void *restoreSceneStateFromData(const void *data, unsigned int flags);
	virtual const void *_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer);
	virtual bool _getTextureForSource(BaseTextureEmu **texture, int source);

	void setParticleTexture(C2DTexture *texture) { LAssign(_texture, texture); }

	Color _color;
	C2DTexture *_texture;
	Vector4 _params;
};

// The atlas's points: the other cities as one point list (its size the
// scene's), the capitals and the Apple stores as textured point sprites.
class CAllCitiesMesh : public CMesh {
	LF_DECLARE_CLASS(CAllCitiesMesh)
public:
	virtual LClass *modelDataClass();
	virtual void drawSubset(CDrawableModelData *data, long subset, long index);
	virtual CAllCitiesMesh *getOwnerLObject() { return this; }
};

class CAllCitiesMeshData : public ModelDataWithMatrix {
	LF_DECLARE_CLASS(CAllCitiesMeshData)
public:
	CAllCitiesMeshData() : _scale(1.0f) {}
	virtual CAllCitiesMeshData *getOwnerLObject() { return this; }
	virtual void storeSceneStateInStream(LData *stream, unsigned int flags);
	virtual const void *restoreSceneStateFromData(const void *data, unsigned int flags);

	float scale() const { return _scale; }
	void setScale(float scale) { _scale = scale; }

	float _scale;
};

class PointsParticleSystem : public CParticleSystem {
	LF_DECLARE_CLASS(PointsParticleSystem)
public:
	virtual LClass *modelDataClass();
	virtual PointsParticleSystem *getOwnerLObject() { return this; }
};

// 'CnSf': (sprite scale, 0, 0, 0).
class PointsModelsDrawableData : public CParticleSystemData {
	LF_DECLARE_CLASS(PointsModelsDrawableData)
public:
	PointsModelsDrawableData() : _params(1, 0, 0, 0) {}
	virtual PointsModelsDrawableData *getOwnerLObject() { return this; }
	virtual const void *_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer);

	Vector4 _params;
};

class CAllPointsContainer : public LObject, public CMeshDataSource, public CSceneStatePropertyOwner {
	LF_DECLARE_CLASS(CAllPointsContainer)
public:
	CAllPointsContainer();
	virtual void dealloc();
	virtual CAllPointsContainer *getOwnerLObject() { return this; }
	virtual intptr_t _onDBChanged(char *args);
	virtual void storeSceneStateInStream(LData *stream, unsigned int flags);
	virtual const void *restoreSceneStateFromData(const void *data, unsigned int flags);
	virtual void renderSelfHolderInScene(CScene *scene, unsigned int flags);
	virtual D3DPRIMITIVETYPE primitiveTypeForMesh(CMesh *mesh);
	virtual unsigned int numPrimitivesForMesh(CMesh *mesh);
	virtual unsigned int numVerticesForMesh(CMesh *mesh);
	virtual void meshWasInstantiated(CMesh *mesh);

	CAllPointsContainer *initWithScene(BaseScene *scene);
	// Draws the points the preferences show, once for each copy of the map.
	void drawWithDrawer(CFrameDrawer *drawer, float pointSize);
	PointsParticleSystem *_createPointsParticleSystem(unsigned int count, LString *texturePath);
	void _fillPointsDrawableData(PointsModelsDrawableData *data, LData *indices);
	static float texturedPointsScaleFromPointSize(float size);

	BaseScene *_scene;	// not retained
	float _pointSize;
	CSharableResource _lock;
	CAllCitiesMesh *_citiesMesh;
	LArray *_citiesDatas;	// one per map copy
	LData *_allPoints;	// SAtlasPoints
	LData *_otherIndices;
	PointsParticleSystem *_capitals;
	LArray *_capitalsDatas;
	PointsParticleSystem *_appleStores;
	LArray *_appleStoresDatas;
};

// The scenes' projections of geographic points.
class CProjectionFlatScene : public CProjection {
	LF_DECLARE_CLASS(CProjectionFlatScene)
public:
	// The map point, z 0.
	virtual void scenePointFromGeoPoint(Vector3 *out, const LPoint *geoPoint);
	virtual CProjectionFlatScene *getOwnerLObject() { return this; }
};

class ProjectionPerspScene : public CProjection {
	LF_DECLARE_CLASS(ProjectionPerspScene)
public:
	// On the ground, in Earth radii.
	virtual void scenePointFromGeoPoint(Vector3 *out, const LPoint *geoPoint);
	virtual ProjectionPerspScene *getOwnerLObject() { return this; }
};

// ---------------------------------------------------------------------------
// Printing (BaseDrawing.mm, SphereDrawing.mm, FlatDrawing.mm): the scene's
// earth, clouds and space rendered into a bitmap, with the overlays (grid,
// borders, points, routes, weather) drawn over it with Cocoa.

// A font for the printed overlays: the system font at a size, in a color
// (CPRFont.mm). The name and flags are kept for the record only.
class CPRFont : public LObject {
	LF_DECLARE_CLASS(CPRFont)
public:
	static const char *kFontDefaultSystemFont;

	CPRFont() : _size(0), _attributes(0) {}
	virtual void dealloc();
	virtual CPRFont *getOwnerLObject() { return this; }

	// Autoreleased.
	static CPRFont *fontWithParams(float size, const char *name, unsigned int flags, const Color &color,
	                               void *reserved);
	CPRFont *initWithParams(float size, const char *name, unsigned int flags, const Color &color, void *reserved);
	void drawString(LString *string, const LPoint &point, LSize *reserved);
	LSize stringSize(LString *string);
	float size() const { return _size; }

	float _size;
	LObjToNSObjWrapper *_attributes;	// the NSDictionary of the string attributes
};

// The printing flags (BaseDrawing::defaultPrintingFlags).
enum {
	kPrintFlagEarth = 1,	// kPrintEarth
	kPrintFlagClouds = 2,	// kPrintClouds
	kPrintFlagDayNight = 4,
	kPrintFlagSpace = 8,	// kPrintSpace
	kPrintFlagRoutes = 0x10,
	kPrintFlagRouteLines = 0x20,
	kPrintFlagRouteNames = 0x40,
	kPrintFlagRouteMarkers = 0x80,
	kPrintFlagGrid = 0x100,
	kPrintFlagCountryBorders = 0x1000,
	kPrintFlagCountryNames = 0x2000,
	kPrintFlagStateBorders = 0x10000,
	kPrintFlagStateNames = 0x20000,
	kPrintFlagPoints = 0x100000,
	kPrintFlagPointsOutline = 0x200000,
	kPrintFlagWeather = 0x20000000,
};

class BaseDrawing : public LObject {
	LF_DECLARE_ABSTRACT_CLASS(BaseDrawing)
public:
	// An image to draw at a point, `size` view units big.
	struct SImageDrawData {
		LImage *image;
		LPoint geoPoint;
		LSize size;
	};

	BaseDrawing();
	virtual void dealloc();
	virtual BaseDrawing *initWithFrameSizeScene(LSize size, BaseScene *scene, unsigned int flags);
	virtual LPoint viewPointFromGeoPoint(const LPoint &geoPoint, PanelDrawIterator *iterator, unsigned char *visible);
	virtual void drawRect(LRect rect, void *context);
	virtual float scale();
	virtual void setScale(float scale);
	virtual float cityRadius();
	virtual float cityRadiusForWeatherBB();
	virtual Color cityColor();
	virtual Color constellationsColor();
	virtual float routeMarkerRadius();
	virtual CPRFont *routeCaptionFont();
	virtual CPRFont *routeCaptionShadowFont();
	virtual Color lineColor();
	virtual Color gridColor();
	virtual CPRFont *gridMarkerFont();
	virtual Color countryBorderColor();
	virtual CPRFont *planetNameFont();
	virtual CPRFont *planetNameShadowFont();
	virtual CPRFont *constellationNameFont();
	virtual CPRFont *constellationNameShadowFont();
	virtual CPRFont *countryNameFont();
	virtual CPRFont *countryNameShadowFont();
	virtual CPRFont *stateNameFont();
	virtual CPRFont *stateNameShadowFont();
	virtual CPRFont *temperatureFont();
	virtual CPRFont *temperatureShadowFont();
	virtual void adjustClippingsToViewRect(LRect rect);
	virtual bool isGeoRectVisible(LRect geoRect) = 0;
	virtual void drawConstellationsInRect(LRect) {}
	virtual void drawPlanetsNamesInRect(LRect) {}
	virtual void drawConstellationsNamesInRect(LRect) {}
	virtual void drawMapInRect(LRect rect);
	virtual void drawGridInRect(LRect) {}
	virtual void drawCountryBordersInRect(LRect rect);
	virtual void drawStateBordersInRect(LRect rect);
	virtual void drawAllLocationsInRect(LRect rect);
	virtual void drawRoutes();
	virtual void drawRouteMarkersInRect(LRect) {}
	virtual void drawWeatherBillboardsInRect(LRect rect);
	virtual void drawShapeBorder(CShapeContainer *shape);
	virtual void drawShapeName(CShapeContainer *shape);
	virtual void _releasePlatformSpecific();
	virtual void _drawEntryBillboard(GenericEntry *entry, const LRect *rect, PanelDrawIterator *iterator);
	virtual LObjToNSObjWrapper *_transformGeoPath(LObjToNSObjWrapper *path, PanelDrawIterator *iterator);
	virtual void _releasePath(LObjToNSObjWrapper *) {}

	// The flags from the preferences.
	static unsigned int defaultPrintingFlags();
	static bool __measureTime(LDate *, const char *, double *) { return false; }

	BaseScene *scene() const { return _scene; }
	LSize frameSize() const { return _frameSize; }
	float degreesGridStep(unsigned char longitude);
	LImage *_pointIconForKey(LString *key);
	void _drawShapeBordersWithType(LString *shapeID, LRect rect, unsigned char withNames);
	void _drawImagesInRect(SImageDrawData *images, int count, LRect rect);

	// Cocoa.
	void _setPrimaryColor(Color color);
	void _setSecondaryColor(Color color);
	void *_createBitmapWithParams(int width, int height, int rowBytes);
	int _bitmapRowSize();
	void _releaseBitmap();
	void _renderBitmapInRect(LRect rect);
	void _beginDrawingLines(float width);
	void _drawLine(LPoint from, LPoint to);
	void _drawLineTo(LPoint to);
	void _endDrawingLines();
	void _drawString(CPRFont *font, LString *string, const LPoint &point, LSize *size = 0);
	void _drawShapeBorderInRect(LObjToNSObjWrapper *path, LRect rect);
	void _drawFilledEllipseInRect(LRect rect, unsigned char stroke, float lineWidth);
	void _drawImageInRectFromRect(LImage *image, LRect rect, LRect fromRect);

	BaseScene *_scene;	// retained
	AtlasPanelController *_controller;
	Matrix _viewProj;
	unsigned int _flags;	// kPrintFlag...
	LSize _frameSize;
	LRect _clipRect;	// the view rect being drawn
	LRect _drawRect;
	float _scale;
	id _linesPath;	// NSBezierPath between _beginDrawingLines and _endDrawingLines
	int _f90;
	LMap *_pointIcons;	// LImages by sPointIconKeys
	id _primaryColor;	// NSColor
	id _secondaryColor;
	id _bitmap;	// NSBitmapImageRep of the rendered scene
};

class SphereDrawing : public BaseDrawing {
	LF_DECLARE_CLASS(SphereDrawing)
public:
	SphereDrawing();
	virtual void dealloc();
	virtual SphereDrawing *initWithFrameSizeScene(LSize size, BaseScene *scene, unsigned int flags);
	virtual bool isGeoRectVisible(LRect) { return true; }
	virtual void drawConstellationsInRect(LRect rect);
	virtual void drawPlanetsNamesInRect(LRect rect);
	virtual void drawConstellationsNamesInRect(LRect rect);
	virtual void drawGridInRect(LRect rect);
	virtual void _releasePlatformSpecific();
	virtual SphereDrawing *getOwnerLObject() { return this; }

	// The stars' local space (with the stars' rotation, or for the planets
	// without) scaled inside the camera's far plane.
	const Matrix *_createSpaceMatrix(unsigned char withStarsRotation);
	LPoint _viewPointFromWorldCoords(const Vector3 &v, unsigned char *visible);
	LPoint _viewPointFromLocalCoords(const Vector3 &v, const Matrix &m, unsigned char *visible);
	// The earth's center and the four points of its outline (left, top,
	// right, bottom from the camera), and the camera direction's cosine to
	// the earth.
	void _calculateEarthPoints(LPoint &center, LPoint &p0, LPoint &p1, LPoint &p2, LPoint &p3, float &cosine);
	// Clips to the space around the earth.
	void _applyEarthClipRegion();
	static void _saveGraphicsState();
	static void _restoreGraphicsState();

	id _earthClipPath;	// NSBezierPath
	Matrix _spaceMatrix;
	bool _spaceMatrixDirty;
	bool _spaceMatrixWithStars;
};

class FlatDrawing : public BaseDrawing {
	LF_DECLARE_CLASS(FlatDrawing)
public:
	FlatDrawing() {}
	virtual void dealloc();
	virtual FlatDrawing *initWithFrameSizeScene(LSize size, BaseScene *scene, unsigned int flags);
	virtual LPoint viewPointFromGeoPoint(const LPoint &geoPoint, PanelDrawIterator *iterator, unsigned char *visible);
	virtual void adjustClippingsToViewRect(LRect rect);
	virtual bool isGeoRectVisible(LRect geoRect);
	virtual void drawGridInRect(LRect rect);
	virtual FlatDrawing *getOwnerLObject() { return this; }
	// The path in view coordinates, without the jumps across the date line.
	virtual LObjToNSObjWrapper *transformGeoPath(LObjToNSObjWrapper *path, PanelDrawIterator *iterator);

	LRect _geoRect;	// the view's, x from below -180 when it crosses the date line
	LRect _baseGeoRect;
	LRect _clipGeoRect;
};

// AtlasPanelController::drawIterator: a scene is drawn once for each copy
// of the map in view; the iterator steps through them.
class PanelDrawIterator {
public:
	PanelDrawIterator() : _mapOffset(0.0f), _f8(0.0f), _fc(0.0f) {}
	virtual ~PanelDrawIterator() {}
	virtual bool next() = 0;
	// The view space translation of the current copy.
	virtual float viewSpaceTranslationX() { return 0.0f; }

	float _mapOffset;	// in map widths
	float _f8;
	float _fc;
};

// One pass.
class PanelDrawIteratorSimple : public PanelDrawIterator {
public:
	PanelDrawIteratorSimple() : _done(false) {}
	virtual bool next()
	{
		bool first = !_done;
		_done = true;
		return first;
	}

	bool _done;
};

// ---------------------------------------------------------------------------
// The scene's own state for recorded movies: the camera parameters, the
// highlighted shape and the shapes caption.

class SceneState : public LObject, public CSceneStatePropertyOwner {
	LF_DECLARE_CLASS(SceneState)
public:
	SceneState() : _scene(0) {}
	virtual SceneState *getOwnerLObject() { return this; }
	virtual void storeSceneStateInStream(LData *stream, unsigned int flags);
	virtual const void *restoreSceneStateFromData(const void *data, unsigned int flags);
	virtual void renderSelfHolderInScene(CScene *, unsigned int) {}

	SceneState *initWithScene(BaseScene *scene)
	{
		_scene = scene;
		return this;
	}

	BaseScene *_scene;
};

class ShapeContainerStateHolder : public LObject, public CSceneStatePropertyOwner {
	LF_DECLARE_CLASS(ShapeContainerStateHolder)
public:
	ShapeContainerStateHolder() : _scene(0), _shapeID(0), _objectUID(0), _caption(0) {}
	virtual void dealloc();
	virtual ShapeContainerStateHolder *getOwnerLObject() { return this; }
	virtual void storeSceneStateInStream(LData *stream, unsigned int flags);
	virtual const void *restoreSceneStateFromData(const void *data, unsigned int flags);
	virtual void renderSelfHolderInScene(CScene *scene, unsigned int flags);

	ShapeContainerStateHolder *initWithScene(BaseScene *scene)
	{
		_scene = scene;
		return this;
	}
	void grabInfoFromShapeContainer(CShapeContainer *container, const LString *caption);

	BaseScene *_scene;
	LString *_shapeID;
	LString *_objectUID;
	LString *_caption;
};

class ShapesCaptionStateHolder : public LObject, public CSceneStatePropertyOwner {
	LF_DECLARE_CLASS(ShapesCaptionStateHolder)
public:
	ShapesCaptionStateHolder() : _scene(0), _shapeID(0), _objectUID(0), _caption(0) {}
	virtual void dealloc();
	virtual ShapesCaptionStateHolder *getOwnerLObject() { return this; }
	virtual void storeSceneStateInStream(LData *stream, unsigned int flags);
	virtual const void *restoreSceneStateFromData(const void *data, unsigned int flags);
	virtual void renderSelfHolderInScene(CScene *scene, unsigned int flags);

	ShapesCaptionStateHolder *initWithScene(BaseScene *scene);
	void setShapesInfo(CShapeContainer *container, LString *caption);

	BaseScene *_scene;
	LString *_shapeID;
	LString *_objectUID;
	LString *_caption;
};

// 'fcvl': 1 / the far clip and that - 1, for the shaders that fade at the
// far plane.
class CClipingShaderStage : public CShaderStage {
	LF_DECLARE_CLASS(CClipingShaderStage)
public:
	CClipingShaderStage();
	virtual CClipingShaderStage *getOwnerLObject() { return this; }
	virtual const void *_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer);

	static void setFarClipValue(float value);
	static float sFarClipValue[4];
};

// ---------------------------------------------------------------------------
// BaseScene

class BaseScene : public CScene, public CMeshDataSource {
	LF_DECLARE_ABSTRACT_CLASS(BaseScene)
public:
	// _flags
	enum { kSceneHasClouds = 2, kSceneHasStraightedge = 4 };

	// Turns off the refresh on world time changes for its scope.
	class StDisableRefreshOnTimeChange {
	public:
		StDisableRefreshOnTimeChange(BaseScene &scene);
		~StDisableRefreshOnTimeChange() { _scene._refreshOnTimeChange = _saved; }

		BaseScene &_scene;
		bool _saved;
	};

	BaseScene();
	virtual void dealloc();
	virtual BaseScene *getOwnerLObject() { return this; }
	virtual void storeCaptureSessionConstants(CCaptureSession *session);
	virtual void setUseConstantsFromSession(CCaptureSession *session);
	virtual void renderAllDrawables(CFrameDrawer *drawer);
	virtual void frameWillBeRendered();
	virtual void drawScene();
	virtual bool processMessage(unsigned int message, unsigned int wParam, unsigned int lParam);
	virtual void mouseDown(unsigned int flags, LPoint point);
	virtual void mouseMove(unsigned int flags, LPoint point);
	virtual void mouseUp(unsigned int flags, LPoint point);
	virtual void onMouseLeave(unsigned int flags, LPoint point);
	virtual void mouseDblClick(unsigned int flags, LPoint point);
	virtual void mouseWheel(short delta, unsigned int flags, LPoint point);
	virtual void mouseDragged(unsigned int flags, LPoint point);
	virtual void dragStarted(unsigned int flags, LPoint point);
	virtual void dragFinished(unsigned int flags, LPoint point);
	virtual bool canBecameInFocus() { return true; }
	virtual bool becameInFocus();
	virtual bool resignFocus();
	virtual bool onKeyDown(unsigned int key, unsigned int flags);
	virtual bool onKeyUp(unsigned int key, unsigned int flags);
	virtual CSceneStatePropertyOwner *stateOwnerWithName(const char *name);

	virtual BaseScene *initWithController(AtlasPanelController *controller, CHolder *holder);	// 0x110
	virtual void hostControllerWasInstalled();
	virtual Color fogColor();
	virtual void viewFrameChanged() {}
	virtual void worldTimeChanged();
	virtual void highlightEntry(GenericEntry *entry, unsigned char highlight);
	virtual void dragBringsEntries(TLArray<LMap *> *) {}
	virtual void dragExitedWithAllEntries() {}
	virtual void scenePointFromGeoPoint(Vector3 *out, const LPoint *geoPoint);	// 0x130
	virtual bool geoPointFromNormalizedViewPoint(LPoint *point) = 0;
	virtual bool geoRectIsVisible(LRect rect) = 0;
	// The autoreleased particle system for `count` route points, or 0.
	virtual CParticleSystem *routePointsParticleSystem(int count) = 0;
	virtual bool viewPointFromGeoPoint(LPoint *out, const LPoint *geoPoint, PanelDrawIterator *iterator,
	                                   const LSize *viewSize) const;	// 0x140
	virtual float triangleDetailing() { return 0.0f; }
	virtual CProjection *_createProjection() = 0;
	virtual void installCinematicSequence(CinematicSequence *sequence);
	virtual void _renderEnvironment(CFrameDrawer *) {}	// 0x150
	virtual void _renderEarthMap(CFrameDrawer *drawer) = 0;
	virtual void _renderOverlays(CFrameDrawer *drawer);
	virtual void _renderAnimatedMarker(LPoint geoPoint, CFrameDrawer *drawer);
	virtual void _renderMinimap(CFrameDrawer *drawer);	// 0x160
	virtual void _renderNavDevice(CFrameDrawer *drawer);
	virtual void _renderInfoDevice(CFrameDrawer *drawer);
	virtual void _renderEntriesBillboards(CFrameDrawer *drawer);
	// `geoRect` bounds the shape, `labelGeoRect` its largest path (both
	// geographic).
	virtual LPoint _viewMapCaptionPositionForPolysContainer(const LSize &captionSize, const LRect &geoRect,
	                                                        const LRect &labelGeoRect, PanelDrawIterator *iterator);	// 0x170
	// The caption's view position from the shape's quad trees (east and west
	// halves): at the largest visible part nearest `labelGeoRect`'s center;
	// -10000s when it is out of view.
	virtual LPoint _viewMapCaptionPosForQuadTrees(const LSize &captionSize, QuadTreeContainer *east,
	                                              QuadTreeContainer *west, const LRect &geoRect,
	                                              const LRect &labelGeoRect, PanelDrawIterator *iterator);
	virtual void _updateCamera();
	virtual void _updateWorldTimeSettings();
	virtual void _adjustCameraToParameters(LPoint position, float scale) = 0;	// 0x180
	virtual void _recacheCameraDependedObjects();
	// The straightedge mesh.
	virtual D3DPRIMITIVETYPE primitiveTypeForMesh(CMesh *mesh);
	virtual unsigned int numPrimitivesForMesh(CMesh *mesh);
	virtual unsigned int numVerticesForMesh(CMesh *mesh);	// 0x190
	virtual void meshWasInstantiated(CMesh *mesh);
	virtual void _adjustCloudsMeshData(CCloudsMeshData *, unsigned char, PanelDrawIterator *) {}
	virtual void _adjustAllCitiesData(CAllCitiesMeshData *data, PanelDrawIterator *iterator);
	virtual void _adjustAllPointsData(ModelDataWithMatrix *data, PanelDrawIterator *iterator);	// 0x1a0
	virtual void _adjustGridData(ModelDataWithMatrix *data, PanelDrawIterator *iterator);
	virtual void _adjustRouteData(ModelDataWithMatrix *data, PanelDrawIterator *iterator);
	virtual void _adjustShapeData(ModelDataWithMatrix *data, PanelDrawIterator *iterator);
	virtual void _adjustRoutePointsProxy(CParticleSystemDrawProxy *proxy, PanelDrawIterator *iterator);	// 0x1b0
	virtual void storeSceneStateInStream(LData *stream, unsigned int flags);
	virtual const void *loadSceneStateFromStream(const void *data, unsigned int flags);
	virtual void _renderScale(CFrameDrawer *) {}
	virtual float _premulAngleFactor() { return 0.0244346f; }	// 0x1c0

	void invalidate();
	CinematicSequence *currentCinematicSequence() const { return _cinematic; }
	void startCountdown(int seconds);
	void stopCountdown();
	void viewProjMatrixForViewWithSize(Matrix *out, LSize size);
	SharedContextObjectsContainer *objectsContainer();
	AtlasPanelController *controller() const { return _controller; }
	CProjection *projection() const { return _projection; }
	RenderOptions *renderOptions() const { return _renderOptions; }
	void setRenderOptions(RenderOptions *options);
	bool viewPointFromGeoPointSmartProject(LPoint *out, const LPoint *geoPoint, PanelDrawIterator *iterator) const;
	void setSelectionRect(LRect rect);
	void _fillStraightedgeMesh(CMesh *mesh, float length);
	void _renderShapeContainer(CFrameDrawer *drawer, CShapeContainer *container, const LString *caption,
	                           unsigned char highlighted);
	void _renderShapesCaption(CFrameDrawer *drawer, LString *caption, CShapeContainer *container,
	                          unsigned char highlighted);

	AtlasPanelController *_controller;
	CProjection *_projection;
	CinematicSequence *_cinematic;
	RenderOptions *_renderOptions;
	GridLineMesh *_gridMesh;
	CGridMarkers *_gridMarkers;	// the subclass's
	CCloudsMesh *_cloudsMesh;
	CMeshDataSource *_cloudsDataSource;	// the subclass's; an LObject
	CAllPointsContainer *_allPoints;
	GenericEntry *_highlightedEntry;
	CFontDrawableData *_entryCaptionFont;	// the highlighted entry's name
	bool _highlighted;
	ModelDataWithMatrix *_zoomRectData;
	CMesh *_zoomRectMesh;
	double _countdownStart;
	int _countdownSeconds;
	LObject *_unusedDC;	// released, never set
	CFontDrawableData *_statusFont;	// "Authorizing", "Downloading Clouds"
	CBillboardModelData *_statusBillboard;
	CFontDrawableData *_boldInfoFont;
	CFontDrawableData *_normalInfoFont;
	LObject *_unusedF0[9];	// released, never set
	CMesh *_straightedgeMesh;
	CDrawableModelData *_straightedgeData;
	SceneState *_sceneState;
	ShapeContainerStateHolder *_shapeContainerState;
	ShapesCaptionStateHolder *_shapesCaptionState;
	unsigned int _flags;
	float _farClipValue;
	bool _flag130;
	CRenderProfile *_routePointsProfile;
	bool _timeFrozen;
	bool _refreshOnTimeChange;
	LPoint _point13C;
	// The controller's view when the camera depended objects were last
	// recached (-1s at first).
	LPoint _cachedPosition;
	float _cachedScale;
	float _cachedCompassAngle;
	CFontDrawableData *_shapesCaptionFont;
};

// ---------------------------------------------------------------------------
// PerspectiveScene: the globe with its sky, stars, lights and eclipses.

class PerspectiveScene : public BaseScene {
	LF_DECLARE_CLASS(PerspectiveScene)
public:
	PerspectiveScene();
	virtual void dealloc();
	virtual PerspectiveScene *getOwnerLObject() { return this; }
	virtual void playbackStoppedWithCapturer(CSceneCapturer *capturer);
	virtual void renderAllDrawables(CFrameDrawer *drawer);
	virtual CSceneStatePropertyOwner *stateOwnerWithName(const char *name);
	virtual PerspectiveScene *initWithController(AtlasPanelController *controller, CHolder *holder);
	virtual Color fogColor() { return _fogColor; }
	virtual void dragBringsEntries(TLArray<LMap *> *entries);
	virtual void dragExitedWithAllEntries();
	virtual bool geoPointFromNormalizedViewPoint(LPoint *point);
	virtual bool geoRectIsVisible(LRect) { return true; }
	virtual CParticleSystem *routePointsParticleSystem(int count);
	virtual float triangleDetailing() { return 3.0f; }
	virtual CProjection *_createProjection();
	virtual void _renderEnvironment(CFrameDrawer *drawer);
	virtual void _renderEarthMap(CFrameDrawer *drawer);
	virtual LPoint _viewMapCaptionPositionForPolysContainer(const LSize &captionSize, const LRect &geoRect,
	                                                        const LRect &labelGeoRect, PanelDrawIterator *iterator);
	virtual void _updateWorldTimeSettings();
	virtual void _adjustCameraToParameters(LPoint position, float scale);
	virtual void _adjustCloudsMeshData(CCloudsMeshData *data, unsigned char high, PanelDrawIterator *iterator);
	virtual void _adjustRoutePointsProxy(CParticleSystemDrawProxy *proxy, PanelDrawIterator *iterator);
	virtual void _renderAbout(CFrameDrawer *drawer);	// 0x1c4

	void _parseASEFile(LString *path, unsigned char arg1, unsigned char arg2);
	Stars *stars() const { return _stars; }
	NatureParams *natureParams() const { return _natureParams; }
	Atmosphere *atmosphere() const { return _atmosphere; }

	Stars *_stars;
	NatureParams *_natureParams;
	Atmosphere *_atmosphere;
	Color _fogColor;
	AdaptiveLODSphere *_sphere;
	CContextLight *_sunLight;
	CContextLight *_light1;
	CContextLight *_light2;
	CEclipseMeshData *_eclipseData;
	CEclipseMesh *_eclipseMesh;
	_EclipseHemisphereDataSource *_eclipseSource;
	LArray *_aseVertices;	// _parseASEFile's
	LArray *_aseFaces;
	float _starsScale;
	LObject *_unused19C;	// released, never set
	LObject *_unused1A0;
	CParticleSystem *_routePointsSystem;	// the last routePointsParticleSystem
	CPushpinModel *_pushpinModel;
	ModelDataWithMatrix *_aboutData;
	double _aboutStartTime;	// -1 before the first frame
};

// ---------------------------------------------------------------------------
// The flat map's tiles: a quad per patch of the earth texture.

class FlatEntryObjectData : public ModelDataWithMatrix {
	LF_DECLARE_CLASS(FlatEntryObjectData)
public:
	FlatEntryObjectData() : _loader(0), _alpha(0.0f)
	{
		_id.x = 0;
		_id.y = 0;
		_id.level = 0;
	}
	virtual FlatEntryObjectData *getOwnerLObject() { return this; }
	// 'TTrn', 'TTro': the texture and city lights transforms; 'TTrp': the
	// city lights color.
	virtual const void *_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer);
	// Source 0: the patch's texture; 1: its city lights.
	virtual bool _getTextureForSource(BaseTextureEmu **texture, int source);

	// Places the tile of the patch, shifted by `offsetX` maps, and requests
	// its textures; `lights` is the city lights' strength.
	void setDrawParameters(const STextureID &id, CTextureLoader *loader, float offsetX, float lights);

	CTextureLoader *_loader;
	STextureID _id;
	STextureUnitTransforms _texture;
	STextureUnitTransforms _glows;
	float _alpha;
};

class FlatEntryMesh : public CMesh {
	LF_DECLARE_CLASS(FlatEntryMesh)
public:
	virtual LClass *modelDataClass();
	// Always visible: the renderer places only visible tiles.
	virtual bool evalVisibilityParams(CDrawableModelData *data, Vector3 *center, float *distance);
	virtual FlatEntryMesh *getOwnerLObject() { return this; }
};

// ---------------------------------------------------------------------------
// FlatMapRenderer: draws the flat map's tiles at the detail the zoom needs,
// the night side over them, and the moon's shadow during eclipses.

class FlatMapScene;

class FlatMapRenderer : public LObject, public CSceneStatePropertyOwner {
	LF_DECLARE_CLASS(FlatMapRenderer)
public:
	FlatMapRenderer();
	virtual void dealloc();
	virtual FlatMapRenderer *getOwnerLObject() { return this; }
	virtual void storeSceneStateInStream(LData *stream, unsigned int flags);
	virtual const void *restoreSceneStateFromData(const void *data, unsigned int flags);
	virtual void renderSelfHolderInScene(CScene *scene, unsigned int flags);

	FlatMapRenderer *initWithScene(FlatMapScene *scene);
	// Takes the sun, the moon and the options, and draws.
	void render(CFrameDrawer *drawer);
	void _render(CFrameDrawer *drawer);
	// Shades the separator grid's vertices by the sun when the view moved;
	// `nightAlpha` is how dark the night side is.
	void _recalcSeparatorNormalsIfNeeded(float nightAlpha);

	FlatMapScene *_scene;
	CMesh *_separatorMesh;
	CDrawableModelData *_separatorData;
	LArray *_entries;	// FlatEntryObjectData, reused each frame
	FlatEntryMesh *_entryMesh;
	CEclipseMesh *_eclipseMesh;
	LArray *_eclipseDatas;	// CEclipseMeshData, one per copy of the map
	bool _separatorValid;	// false: shade again
	Vector3 _sunDirection;
	Vector3 _moonPosition;
	float _nightAlpha;
	bool _showDayNight;
};

// ---------------------------------------------------------------------------
// FlatMapScene: the flat map, seen through an orthographic camera.

class FlatMapRenderer;
class _EclipseFlatDataSource;
class GlowDrawableData;

class FlatMapScene : public BaseScene {
	LF_DECLARE_CLASS(FlatMapScene)
public:
	FlatMapScene();
	virtual void dealloc();
	virtual FlatMapScene *getOwnerLObject() { return this; }
	virtual FlatMapScene *initWithController(AtlasPanelController *controller, CHolder *holder);
	virtual void viewFrameChanged();
	virtual bool geoPointFromNormalizedViewPoint(LPoint *) { return false; }
	virtual bool geoRectIsVisible(LRect rect);
	virtual CParticleSystem *routePointsParticleSystem(int count);
	virtual float triangleDetailing() { return 10000.0f; }
	virtual CProjection *_createProjection();
	virtual void _renderEarthMap(CFrameDrawer *drawer);
	virtual void _updateWorldTimeSettings();
	virtual void _adjustCameraToParameters(LPoint position, float scale);
	virtual void _adjustCloudsMeshData(CCloudsMeshData *data, unsigned char high, PanelDrawIterator *iterator);
	virtual void storeSceneStateInStream(LData *stream, unsigned int flags);
	virtual const void *loadSceneStateFromStream(const void *data, unsigned int flags);
	virtual float _premulAngleFactor() { return 0.0027777778f; }

	// The day and night line is drawn again.
	void invalidateDayNightSeparator();

	FlatMapRenderer *_renderer;
	_EclipseFlatDataSource *_eclipseSource;
};

// ---------------------------------------------------------------------------
// LogoScene: the 24-second logo: the big Earth turning under the sun, the
// stars, the title and the flares, then the logo rectangle and a fade out.

// The logo made of points: a grid as wide as `columns`, colored by its
// texture coordinates, scattered towards the camera.
class LogoSystem : public LObject {
	LF_DECLARE_CLASS(LogoSystem)
public:
	LogoSystem() {}
	virtual void dealloc();
	virtual LogoSystem *getOwnerLObject() { return this; }

	LogoSystem *initWithParams(CHolder *holder, LString *texturePath, int columns);
	// Made on first use; 0 without the texture.
	CParticleSystemData *modelData();
	// Width / height of the logo texture.
	float aspect();

	LString *_texturePath;
	CHolder *_holder;
	CParticleSystemData *_data;
	float _aspect;
	int _columns;
};

class LogoScene : public BaseScene {
	LF_DECLARE_CLASS(LogoScene)
public:
	LogoScene();
	virtual void dealloc();
	virtual LogoScene *getOwnerLObject() { return this; }
	virtual LogoScene *initWithController(AtlasPanelController *controller, CHolder *holder);
	virtual bool geoPointFromNormalizedViewPoint(LPoint *) { return false; }
	virtual bool geoRectIsVisible(LRect) { return false; }
	virtual CParticleSystem *routePointsParticleSystem(int) { return 0; }
	virtual CProjection *_createProjection() { return 0; }
	virtual void _renderEarthMap(CFrameDrawer *drawer);
	virtual void _adjustCameraToParameters(LPoint position, float scale);

	static LString *kLogoSceneWasEndedNotification;

	// Starts over.
	void rewind();
	// Jumps to the end and posts kLogoSceneWasEndedNotification.
	void stopDemo();
	CBillboardModelData *detachBillboardData();
	CBillboardModelData *logoRectDrawableData();
	GlowDrawableData *sunGlowDrawableData();

	bool _ended;	// kLogoSceneWasEndedNotification was posted
	double _startTime;	// -1 before the first frame
	CContextLight *_sunLight;
	CContextLight *_titleLight;
	CGlowModelData *_lensFlares;
	GlowDrawableData *_sunGlow;	// made when the flares first show
	float _flaresVisibility;
	float _fade;	// 0 black .. 1
	float _logoRectAlpha;
	CParticleSystemData *_stars;
	LogoSystem *_logoSystem;
	CBillboardModel *_billboardModel;
	CBillboardModelData *_logoRect;
	ModelDataWithMatrix *_earth;
	GlowDrawableData *_earthGlow;
	ModelDataWithMatrix *_title;
};

// ---------------------------------------------------------------------------
// SharedContextObjectsContainer: made once a view (the holder's object for
// kContextObjectsContainerKey); every accessor makes its object on first use.

enum TCompassTextureID { kCompassTexturesCount = 0x17 };

class SharedContextObjectsContainer : public LObject, public CMeshDataSource {
	LF_DECLARE_CLASS(SharedContextObjectsContainer)
public:
	SharedContextObjectsContainer();
	virtual void dealloc();
	virtual SharedContextObjectsContainer *getOwnerLObject() { return this; }
	virtual intptr_t _entryInfoWasChanged(char *args);
	// The city marker quad.
	virtual D3DPRIMITIVETYPE primitiveTypeForMesh(CMesh *) { return D3DPT_TRIANGLELIST; }
	virtual unsigned int numPrimitivesForMesh(CMesh *mesh) { return mesh == (CMesh *)_markerMesh ? 2 : 0; }
	virtual unsigned int numVerticesForMesh(CMesh *mesh) { return mesh == (CMesh *)_markerMesh ? 4 : 0; }
	virtual void meshWasInstantiated(CMesh *mesh);

	SharedContextObjectsContainer *initWithHolderAndWndInterface(CHolder *holder, IWindowInterface *wnd);
	void setLockButtonsTransparency(float transparency);
	void setSceneForInternalObjects(BaseScene *scene);
	void setImagesEntry(GenericEntry *entry);
	void setCurrentEntry(GenericEntry *entry);
	EntryBillboard *billboardsHitTest(LPoint point, BaseScene *scene);
	CHolder *holder() const { return _holder; }
	IWindowInterface *windowInterface() const { return _windowInterface; }
	CTextureLoader *textureLoader() const { return _textureLoader; }
	CloudsController *cloudsController() const { return _cloudsController; }
	GenericEntry *currentEntry() const { return _currentEntry; }
	GenericEntry *imagesEntry() const { return _imagesEntry; }

	RenderOptions *defaultRenderOptions();
	CBillboardModelData *detachBillboardData();
	CBillboardModelData *compassDeviceDrawableData();
	CBillboardModelData *compassCenterDeviceDrawableData();
	CBillboardModelData *compassCircleDeviceDrawableData();
	CBillboardModelData *infoDeviceDrawableData();
	ImagesPreviewController *imagesPreviewController();
	ModelDataWithMatrix *detachMapSelectionDrawableData();
	CRoutePointsContainer *routePointsPreviewContainer();
	CRoutePointsContainer *routePointsContainer();
	EntriesBillboardsHolder *entriesBillboards();
	C2DTexture *explosionTexture();
	C2DTexture *weatherIconsTexture();
	C2DTexture *photoIconBBTexture();
	C2DTexture *weatherBillboardTexture();
	CBillboardModelData *location1DData();
	CBillboardModelData *location0DData();
	C2DTexture *compassDeviceTexture(TCompassTextureID id);
	C2DTexture *infoDeviceTexture(unsigned char full);
	C2DTexture *minimapTexture();
	CBillboardModelData *minimapDrawableData();
	C2DTexture *overlayScaleBackgroundTexture();
	C2DTexture *statusBackgroundTexture();
	CBillboardModelData *clockDigitsDrawableData();
	CBillboardModelData *clockArrowDrawableData();
	CBillboardModelData *clockDrawableData();
	C2DTexture *noiseTexture();
	CBillboardModelData *noiseDrawableData();
	C2DTexture *linesTexture();
	CBillboardModelData *linesDrawableData();
	C2DTexture *whiteRectTexture();
	CBillboardModelData *whiteRectDrawableData();
	C2DTexture *sunSpecularTexture();
	C2DTexture *eclipseTexture();
	C2DTexture *markerTextureCounty();
	C2DTexture *markerTextureCapital();
	C2DTexture *markerTextureApple();
	C2DTexture *markerTexture();
	C2DTexture *weatherMarkerTexture();
	CFontDrawableData *detachDataForBoldInfoFont();
	CFontDrawableData *detachDataForNormalInfoFont();
	CFontDrawableData *detachDataForCaptionFont();
	CFontDrawableData *detachDataForLargeBoldFont();
	CFontDrawableData *detachDataForLargeFont();
	CFontDrawableData *detachDataForSmallFont();
	CFontDrawableData *detachDataForNormalFont();
	CGlowModelData *lensFlares();
	CSceneControl *lockTiltedButton();
	CSceneControl *lockNormalButton();
	CSceneControl *fullScreenButton();
	CSceneControl *minimapButton();
	CSceneControl *infoButton();
	CSceneControl *navButton();
	MarkerModelData *cityMarker();
	AboutMeshData *detachAboutMeshData();

	BaseScene *_scene;
	CTextureLoader *_textureLoader;
	RenderOptions *_defaultRenderOptions;
	CBillboardModel *_billboardModel;
	C2DTexture *_statusBackgroundTexture;
	C2DTexture *_overlayScaleBackgroundTexture;
	C2DTexture *_minimapTexture;
	CBillboardModelData *_minimapData;
	C2DContainer *_mapSelection;
	C2DTexture *_infoShortTexture;
	C2DTexture *_infoFullTexture;
	CBillboardModelData *_infoDeviceData;
	LArray *_compassTextures;	// kCompassTexturesCount, LNull until loaded
	CBillboardModelData *_compassCircleData;
	CBillboardModelData *_compassCenterData;
	CBillboardModelData *_compassDeviceData;
	CBillboardModelData *_location0Data;
	CBillboardModelData *_location1Data;
	CSceneControl *_navButton;
	CSceneControl *_infoButton;
	CSceneControl *_minimapButton;
	CSceneControl *_fullScreenButton;
	CSceneControl *_lockNormalButton;
	CSceneControl *_lockTiltedButton;
	CGlowModelData *_lensFlares;
	CFontDrawable *_smallFont;
	CFontDrawable *_normalFont;
	CFontDrawable *_largeFont;
	CFontDrawable *_largeBoldFont;
	CFontDrawable *_captionFont;
	CFontDrawable *_normalInfoFont;
	CFontDrawable *_boldInfoFont;
	C2DTexture *_eclipseTexture;
	C2DTexture *_sunSpecularTexture;
	C2DTexture *_whiteRectTexture;
	CBillboardModelData *_whiteRectData;
	C2DTexture *_linesTexture;
	CBillboardModelData *_linesData;
	C2DTexture *_noiseTexture;
	CBillboardModelData *_noiseData;
	CBillboardModelData *_clockData;
	CBillboardModelData *_clockArrowData;
	CBillboardModelData *_clockDigitsData;
	MarkerModelData *_cityMarker;
	MarkerMesh *_markerMesh;
	C2DTexture *_markerTexture;
	C2DTexture *_markerTextureApple;
	C2DTexture *_markerTextureCapital;
	C2DTexture *_markerTextureCounty;
	CRoutePointsContainer *_routePoints;
	CRoutePointsContainer *_routePointsPreview;
	CHolder *_holder;
	IWindowInterface *_windowInterface;
	CloudsController *_cloudsController;
	GenericEntry *_currentEntry;
	GenericEntry *_imagesEntry;
	_ImagesPreviewControllerDS *_previewDelegate;
	EntriesBillboardsHolder *_entriesBillboards;
	C2DTexture *_weatherBillboardTexture;
	C2DTexture *_weatherIconsTexture;
	C2DTexture *_photoIconBBTexture;
	C2DTexture *_explosionTexture;
	AboutMesh *_aboutMesh;
	ImagesPreviewController *_imagesPreview;
	bool _flag118;
};

#endif
