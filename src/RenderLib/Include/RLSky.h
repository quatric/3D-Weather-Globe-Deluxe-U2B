// The sky of the perspective scene: the atmosphere dome and its clouds, the
// horizon strips, the sun and moon glows, the moon, the eclipse shadow, and
// the stars, planets and constellation lines.

#ifndef RLSKY_H
#define RLSKY_H

#include "AppModel.h"
#include "FHMeshes.h"
#include "FHParticles.h"
#include "FHSprites.h"
#include "FHTextures.h"

class NatureParams;
class BaseScene;
class StarsModelsDrawableData;
class PerspectiveScene;
class CFontDrawableData;

// ---------------------------------------------------------------------------
// Glows: a cone of rings around a point, facing the camera.

class GlowDrawableData : public ModelDataWithMatrix {
	LF_DECLARE_CLASS(GlowDrawableData)
public:
	// The profile's stages 1..6 are the modes.
	enum TBlendingMode {
		kBlendAlpha = 0,
		kBlendMode1 = 1,
		kBlendMode2 = 2,
		kBlendMode3 = 3,
		kBlendAdd = 4,
		kBlendMode5 = 5,
	};

	GlowDrawableData();
	virtual void storeSceneStateInStream(LData *stream, unsigned int flags);
	virtual const void *restoreSceneStateFromData(const void *data, unsigned int flags);
	// 'CnRt' the cone rotation, 'CnRu' its premultiplied cosines, 'CnRv'
	// and 'CnRw' the dark and light colors, 'CnRx' the prolongation;
	// 39..44 the two texture rotations.
	virtual const void *_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer);
	virtual bool _isStageOn(CShaderStage *stage, unsigned char *on);
	virtual GlowDrawableData *getOwnerLObject() { return this; }

	void setGlowColors(Color light, Color dark)
	{
		_lightColor = light;
		_darkColor = dark;
	}
	void setVisibilityFactor(float f) { _visibility = f; }
	void setDynamicProlongFactor(float f) { _prolong = f; }
	void setBlendingMode(TBlendingMode mode) { _blendMode = mode; }
	// Turns texture `index` (0 or 1) by `turns` of a full circle.
	void setTextureRotation(float turns, int index);
	// Puts the glow of `radius` at `position`, pulled inside the far plane.
	void adjustParameters(const Vector3 *position, float radius, const CContextCamera *camera);
	float visibility() const { return _visibility; }

	int _blendMode;
	Color _darkColor;
	Color _lightColor;
	float _visibility;
	Vector4 _texRotation;	// cos and sin of the two textures' angles
	Matrix _coneRotation;
	Vector4 _conePremul;
	float _prolong;
};

// A sphere of rings: ring r has radius radii[r] and texture coordinate
// texParams[r].
class SphereGlowModel : public CMesh, public CMeshDataSource {
	LF_DECLARE_CLASS(SphereGlowModel)
public:
	enum TexGenMode {
		kTexGenRadial = 0,	// the rings are circles of the texture
		kTexGenCylindric = 1,	// the rings are its rows
	};

	SphereGlowModel();
	virtual void dealloc();
	virtual LClass *modelDataClass();
	virtual bool evalVisibilityParams(CDrawableModelData *data, Vector3 *center, float *distance);
	virtual D3DPRIMITIVETYPE primitiveTypeForMesh(CMesh *) { return D3DPT_TRIANGLELIST; }
	virtual unsigned int numPrimitivesForMesh(CMesh *mesh);
	virtual unsigned int numVerticesForMesh(CMesh *mesh);
	virtual void meshWasInstantiated(CMesh *mesh);
	virtual SphereGlowModel *getOwnerLObject() { return this; }

	SphereGlowModel *initWithParams(unsigned int slices, unsigned int rings, float *radii, float *texParams,
	                                TexGenMode mode, CRenderProfile *profile);

	unsigned int _slices;
	unsigned int _rings;
	float *_radii;
	float *_texParams;	// in _radii's block
	TexGenMode _texGenMode;
};

// ---------------------------------------------------------------------------
// Planets: the moon sphere.

class PlanetDrawableData;

class Planet : public LObject {
	LF_DECLARE_CLASS(Planet)
public:
	Planet();
	virtual void dealloc();
	virtual Planet *getOwnerLObject() { return this; }

	Planet *initWithRenderProfile(CRenderProfile *profile);
	// A new autoreleased data of the planet's sphere.
	PlanetDrawableData *detachDrawableData();

	CRenderProfile *_profile;
	CMesh *_mesh;
};

class PlanetMesh : public CMesh {
	LF_DECLARE_CLASS(PlanetMesh)
public:
	virtual LClass *modelDataClass();
	virtual PlanetMesh *getOwnerLObject() { return this; }
};

class PlanetDrawableData : public ModelDataWithMatrix {
	LF_DECLARE_CLASS(PlanetDrawableData)
public:
	enum TBlendMode { kBlendOpaque = 0, kBlendTransparent = 1 };

	PlanetDrawableData();
	virtual void dealloc();
	// 'PlCo' the diffuse color.
	virtual const void *_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer);
	virtual bool _getTextureForSource(BaseTextureEmu **texture, int source);
	// Turns the sphere by `rotation` and puts it at `position`, pulled to
	// `maxDistance` from the camera.
	virtual void adjustMatrixWithCamera(const Quater &rotation, const Vector3 &position, const Vector3 &camera,
	                                    float maxDistance);
	virtual void setRadius(float radius) { _radius = radius; }
	virtual void setDiffuseTexture(CBaseTexture *texture);
	virtual void setDiffuseColor(const Color &color) { _diffuseColor = color; }
	virtual void setLightingEnabled(unsigned char enabled) { _lighting = enabled; }
	virtual void setBlendMode(TBlendMode mode) { _blendMode = mode; }
	virtual void appendGlowModelData(GlowDrawableData *glow);
	// Stages 1..4: unlit/lit with/without texture, opaque; 5..6
	// unlit/lit transparent.
	virtual bool _isStageOn(CShaderStage *stage, unsigned char *on);
	virtual PlanetDrawableData *getOwnerLObject() { return this; }

	LArray *_glows;	// of GlowDrawableData
	float _radius;
	CBaseTexture *_diffuse;
	Color _diffuseColor;
	bool _lighting;
	int _blendMode;
};

// ---------------------------------------------------------------------------
// The horizon strip around the ground.

class HorizontMesh : public CMesh {
	LF_DECLARE_CLASS(HorizontMesh)
public:
	virtual LClass *modelDataClass();
	virtual bool evalVisibilityParams(CDrawableModelData *data, Vector3 *center, float *distance);
	virtual HorizontMesh *getOwnerLObject() { return this; }
};

class HorizontDrawableData : public ModelDataWithMatrix {
	LF_DECLARE_CLASS(HorizontDrawableData)
public:
	HorizontDrawableData();
	virtual void storeSceneStateInStream(LData *stream, unsigned int flags);
	virtual const void *restoreSceneStateFromData(const void *data, unsigned int flags);
	// 'HoPs' {visibility, factor, 0, 0}, 'HoPt' the color.
	virtual const void *_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer);
	virtual HorizontDrawableData *getOwnerLObject() { return this; }

	float _visibility;
	float _factor;
	Color _color;
};

// ---------------------------------------------------------------------------
// The atmosphere: its dome drawn for the sky and for the cloud layers.

class AtmosphereMesh : public CMesh {
	LF_DECLARE_CLASS(AtmosphereMesh)
public:
	virtual LClass *modelDataClass();
	virtual bool evalVisibilityParams(CDrawableModelData *data, Vector3 *center, float *distance);
	virtual AtmosphereMesh *getOwnerLObject() { return this; }
};

class AtmosphereDrawableData : public ModelDataWithMatrix {
	LF_DECLARE_CLASS(AtmosphereDrawableData)
public:
	// The sky, or a cloud layer.
	enum { kModeSky = 0, kModeClouds = 1 };

	AtmosphereDrawableData();
	virtual void dealloc();
	virtual void storeSceneStateInStream(LData *stream, unsigned int flags);
	virtual const void *restoreSceneStateFromData(const void *data, unsigned int flags);
	// 'AtRt'..'AtR{'.
	virtual const void *_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer);
	virtual bool _isStageOn(CShaderStage *stage, unsigned char *on);
	virtual AtmosphereDrawableData *getOwnerLObject() { return this; }

	// The light and, for clouds, the sky textures: the current or the next
	// sky's; `high` makes a high cloud layer.
	void adjustWithNatureParams(const NatureParams *params, unsigned char next, unsigned char high);
	void adjustToCamera(const CContextCamera *camera);

	Matrix _rotation;	// the dome's, transposed
	float _atmosphereFactor;
	float _sunParam1;
	float _sunParam2;
	float _horizonAngle;
	float _horizonParam;
	Vector3 _sunDirIntersection;
	Vector2 _cloudsOffset;
	Vector2 _cloudsDrift;
	Color _sunColor;
	Color _oppositeColor;
	float _sunsetGlow;
	float _cloudsAlpha;
	int _mode;
	bool _visible;
	double _lastTime;
};

class Atmosphere : public LObject,
                   public C2DTextureDataSource,
                   public CMeshDataSource,
                   public CSceneStatePropertyOwner {
	LF_DECLARE_CLASS(Atmosphere)
public:
	Atmosphere();
	virtual void dealloc();
	virtual Atmosphere *getOwnerLObject() { return this; }
	// The gradient texture: 2 x 64, its alpha rising to the top.
	virtual bool isDynamicTexture(CBaseTexture *, CContext *) { return false; }
	virtual D3DFORMAT textureFormat(CBaseTexture *, unsigned int index, CContext *)
	{
		return index == 0 ? D3DFMT_A8R8G8B8 : (D3DFORMAT)0;
	}
	virtual void textureWasInstantiated(CBaseTexture *texture);
	virtual unsigned int widthOfTexture(C2DTexture *, CContext *) { return 2; }
	virtual unsigned int heightOfTexture(C2DTexture *, CContext *) { return 64; }
	// The dome and the horizon strip.
	virtual D3DPRIMITIVETYPE primitiveTypeForMesh(CMesh *) { return D3DPT_TRIANGLELIST; }
	virtual unsigned int numPrimitivesForMesh(CMesh *mesh);
	virtual unsigned int numVerticesForMesh(CMesh *mesh);
	virtual void meshWasInstantiated(CMesh *mesh);
	virtual void storeSceneStateInStream(LData *stream, unsigned int flags);
	virtual const void *restoreSceneStateFromData(const void *data, unsigned int flags);
	virtual void renderSelfHolderInScene(CScene *scene, unsigned int order);

	Atmosphere *initWithScene(PerspectiveScene *scene);
	void registerDrawDatas(CScene *scene);
	void adjustParameters(const NatureParams *params, const CContextCamera *camera);
	void drawWithDrawer(CFrameDrawer *drawer);
	// 1 on the ground, 0 from 1.02 Earth radii up.
	static float transformFactorFromDistance(float distance);

	PerspectiveScene *_scene;
	CHolder *_holder;
	C2DTexture *_gradient;
	AtmosphereMesh *_mesh;
	AtmosphereDrawableData *_sky;
	AtmosphereDrawableData *_clouds0;
	AtmosphereDrawableData *_clouds1;
	AtmosphereDrawableData *_cloudsHigh;	// never made
	bool _visible;
	HorizontMesh *_horizontMesh;
	GlowDrawableData *_globeGlow;
	HorizontDrawableData *_groundHorizont;
	GlowDrawableData *_sunCircleMain;
	GlowDrawableData *_sunGlowGround;
	GlowDrawableData *_sunCrown;
	GlowDrawableData *_sunGlowSpace;
	float _eclipseFactor;
	PlanetDrawableData *_moon;
	GlowDrawableData *_moonDarkMask;
	GlowDrawableData *_moonCircleMain;
	CGlowModelData *_lensFlares;
};

// ---------------------------------------------------------------------------
// The moon's shadow in an eclipse, and the specular spot on the flat map.

class CEclipseMesh : public CMesh {
	LF_DECLARE_CLASS(CEclipseMesh)
public:
	virtual LClass *modelDataClass();
	virtual bool evalVisibilityParams(CDrawableModelData *data, Vector3 *center, float *distance);
	virtual CEclipseMesh *getOwnerLObject() { return this; }
};

class CEclipseMeshData : public ModelDataWithMatrix {
	LF_DECLARE_CLASS(CEclipseMeshData)
public:
	CEclipseMeshData();
	virtual void storeSceneStateInStream(LData *stream, unsigned int flags);
	virtual const void *restoreSceneStateFromData(const void *data, unsigned int flags);
	// 'ETxM'..'ETxR'.
	virtual const void *_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer);
	virtual bool _isStageOn(CShaderStage *stage, unsigned char *on);
	virtual CEclipseMeshData *getOwnerLObject() { return this; }

	void setVisibility(float visibility) { _visibility = visibility; }
	// The rotation taking y to `v`.
	void _setupRotation(const _SVector3 *v);
	void adjustToLightSpecular(CContextLight *light, const Vector3 *view);
	// From the sun direction and the moon position in kilometers.
	void adjustToSunMoonParams(const Vector3 *sun, const Vector3 *moon);

	bool _shadowStage;
	bool _specularStage;
	float _visibility;
	Vector2 _txt;	// the shadow center in the shadow plane
	bool _inArea;
	Vector3 _sunDirection;
	Vector3 _offset;
	Vector3 _worldX;
	Vector3 _worldY;
};

// The flat map's grid: 65 x 33 vertices with the globe point of each.
class _EclipseFlatDataSource : public LObject, public CMeshDataSource {
	LF_DECLARE_CLASS(_EclipseFlatDataSource)
public:
	virtual D3DPRIMITIVETYPE primitiveTypeForMesh(CMesh *) { return D3DPT_TRIANGLELIST; }
	virtual unsigned int numPrimitivesForMesh(CMesh *) { return 0x1000; }
	virtual unsigned int numVerticesForMesh(CMesh *) { return 0x861; }
	virtual void meshWasInstantiated(CMesh *mesh);
	virtual _EclipseFlatDataSource *getOwnerLObject() { return this; }
};

// A hemisphere a little above the globe, 129 x 33 vertices.
class _EclipseHemisphereDataSource : public LObject, public CMeshDataSource {
	LF_DECLARE_CLASS(_EclipseHemisphereDataSource)
public:
	virtual D3DPRIMITIVETYPE primitiveTypeForMesh(CMesh *) { return D3DPT_TRIANGLELIST; }
	virtual unsigned int numPrimitivesForMesh(CMesh *) { return 0x2000; }
	virtual unsigned int numVerticesForMesh(CMesh *) { return 0x10a1; }
	virtual void meshWasInstantiated(CMesh *mesh);
	virtual _EclipseHemisphereDataSource *getOwnerLObject() { return this; }
};

// ---------------------------------------------------------------------------
// Stars, planets and constellations.

extern const char *kPlanetNames[UPlanets::kPlanetsCount];

class Stars : public LObject, public CSceneStatePropertyOwner {
	LF_DECLARE_CLASS(Stars)
public:
	enum { kPlanetsCount = 8 };
	enum { kNamesConstellations = 1, kNamesPlanets = 2 };

	Stars();
	virtual void dealloc();
	virtual Stars *getOwnerLObject() { return this; }
	virtual void storeSceneStateInStream(LData *stream, unsigned int flags);
	virtual const void *restoreSceneStateFromData(const void *data, unsigned int flags);
	virtual void renderSelfHolderInScene(CScene *scene, unsigned int order);
	virtual intptr_t _planetsParamsWereChanged(char *args);
	virtual intptr_t _worldTimeWasChanged(char *args);

	Stars *initWithHolder(CHolder *holder, BaseScene *scene);
	// `fov` sets the faintest magnitude drawn; the sky is a sphere of
	// `scale` around `position`.
	void drawWithDrawer(float fov, float scale, const Vector3 *position, CFrameDrawer *drawer);
	static const char *planetName(unsigned int planet);

	void _registerDrawDatas(BaseScene *scene);
	void _updatePlanets();
	void _adjustToCurrentDate();
	void _render(CFrameDrawer *drawer);
	void _renderNames(CFrameDrawer *drawer);

	BaseScene *_scene;
	StarsModelsDrawableData *_stars;
	StarsModelsDrawableData *_planets;
	ModelDataWithMatrix *_constellations;
	float _planetMagnitudes[kPlanetsCount];
	Vector3 _planetDirections[kPlanetsCount];
	bool _planetsComputed;
	bool _planetsUploaded;
	Matrix _starsRotation;
	Matrix _planetsRotation;
	Matrix _starsWorld;
	Matrix _planetsWorld;
	int _namesFlags;
	bool _stored;	// the state holds the names flags, not the planets
};

// The visible patches of the star catalog.
class StarsModelsDrawableData : public CParticleSystemData {
	LF_DECLARE_CLASS(StarsModelsDrawableData)
public:
	StarsModelsDrawableData();
	// 'CnSm' the magnitudes, 'CnSn' the billboard corners.
	virtual const void *_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer);
	virtual StarsModelsDrawableData *getOwnerLObject() { return this; }

	void fillVisibleEntries(CContext *context);

	Vector4 _magnitude;	// faintest, fully bright
	TArray<AtlasDatabase::SStarsPathID> _visiblePatches;
};

class StarsParticleSubSystem : public CParticleSubsystem {
	LF_DECLARE_CLASS(StarsParticleSubSystem)
public:
	// Only the visible patches.
	virtual void drawSubset(CDrawableModelData *data, long subset, long index);
	virtual StarsParticleSubSystem *getOwnerLObject() { return this; }
};

class StarsParticleSystem : public CParticleSystem {
	LF_DECLARE_CLASS(StarsParticleSystem)
public:
	StarsParticleSystem() : _usePatches(false) {}
	virtual LClass *modelDataClass();
	virtual void doImmediateDrawWithData(CDrawableModelData *data, CFrameDrawer *drawer);
	virtual StarsParticleSystem *getOwnerLObject() { return this; }

	bool _usePatches;	// the stars, drawn by catalog patches
};

class ConstellMesh : public CMesh {
	LF_DECLARE_CLASS(ConstellMesh)
public:
	virtual bool evalVisibilityParams(CDrawableModelData *data, Vector3 *center, float *distance);
	virtual ConstellMesh *getOwnerLObject() { return this; }
};

// The constellation lines from AtlasDatabase.
class ConstellLinesDataSource : public LObject, public CMeshDataSource {
	LF_DECLARE_CLASS(ConstellLinesDataSource)
public:
	ConstellLinesDataSource() : _stars(0), _indices(0) {}
	virtual ConstellLinesDataSource *init();
	virtual void dealloc();
	virtual D3DPRIMITIVETYPE primitiveTypeForMesh(CMesh *) { return D3DPT_LINELIST; }
	virtual unsigned int numPrimitivesForMesh(CMesh *mesh);
	virtual unsigned int numVerticesForMesh(CMesh *mesh);
	virtual void meshWasInstantiated(CMesh *mesh);
	virtual ConstellLinesDataSource *getOwnerLObject() { return this; }

	LData *_stars;
	LData *_indices;
};

// ---------------------------------------------------------------------------
// Clouds: the frames of the clouds map, drawn as two layers over the earth.

class CCloudsMesh : public CMesh {
	LF_DECLARE_CLASS(CCloudsMesh)
public:
	virtual LClass *modelDataClass();
	virtual bool evalVisibilityParams(CDrawableModelData *data, Vector3 *center, float *distance);
	virtual CCloudsMesh *getOwnerLObject() { return this; }
};

class CCloudsMeshData : public ModelDataWithMatrix {
	LF_DECLARE_CLASS(CCloudsMeshData)
public:
	CCloudsMeshData();
	virtual void storeSceneStateInStream(LData *stream, unsigned int flags);
	virtual const void *restoreSceneStateFromData(const void *data, unsigned int flags);
	// 'blfc'.
	virtual const void *_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer);
	virtual CCloudsMeshData *getOwnerLObject() { return this; }

	// How far the next frame is blended in.
	void adjustWithBlendFactor(float factor);
	void setVisibility(float visibility);

	float _blendFactor;
	float _visibility;
};

// Answers the "use blending" variable with the clouds animation quality.
class CCloudsShaderStage : public CShaderStage {
	LF_DECLARE_CLASS(CCloudsShaderStage)
public:
	virtual const int *addressOfVariable(LString *name);
	virtual CCloudsShaderStage *getOwnerLObject() { return this; }

	static int cloudsAnimationQuality;
};

// Keeps the textures of the current and the next clouds frame, for the
// eastern and the western hemisphere, and steps through the frames.
class CloudsController : public LObject, public CSceneStatePropertyOwner {
	LF_DECLARE_CLASS(CloudsController)
public:
	static LString *kCloudsFrameIndexKey;
	static LString *kCloudsFrameWasChanged;
	static LString *kCloudsFrameBITOffsetKey;

	CloudsController();
	virtual void dealloc();
	virtual CloudsController *getOwnerLObject() { return this; }
	virtual void renderInScene(BaseScene *scene, CFrameDrawer *drawer);
	virtual intptr_t _contextWasAttached(char *args);
	virtual intptr_t _cloudsTextureSizeWasChanged(char *args);
	virtual intptr_t _cloudsEnableStateWasChanged(char *args);
	virtual void storeSceneStateInStream(LData *stream, unsigned int flags);
	virtual const void *restoreSceneStateFromData(const void *data, unsigned int flags);
	virtual void renderSelfHolderInScene(CScene *scene, unsigned int flags);

	CloudsController *initWithHolder(CHolder *holder);
	void invalidateCloudsTextures();
	void invalidateAnimationParams();
	void pauseCloudsAnimation() { _paused = true; }
	void resumeCloudsAnimation() { _paused = false; }
	// In hours, of the frame shown; -1 without clouds.
	int currentBackInTimeOffset();
	void setBackInTimeOffset(int hours);
	C2DTexture *textureForEast();
	C2DTexture *textureForWest();
	C2DTexture *textureForEastNext();
	C2DTexture *textureForWestNext();

	CCloudsMeshData *_detachCloudsMeshData(CCloudsMesh *mesh, unsigned char current);
	void _render(BaseScene *scene, CFrameDrawer *drawer);
	void findCloudsTextureFormat();
	C2DTexture *_createCloudsTexture();
	// Makes the next textures current; returns whether one was missing.
	bool swapCloudsTextures();
	// Returns whether there is a frame to draw.
	bool _adjustCloudsToFrame(int frame, TCloudsAnimation animation);

	CHolder *_holder;
	C2DTexture *_east;
	C2DTexture *_west;
	C2DTexture *_eastNext;
	C2DTexture *_westNext;
	double _frameStart;
	bool _invalid;
	float _blendFactor;
	float _visibility;
	int _frame;
	bool _paused;
	TCloudsAnimation _animation;
	int _shownFrame;
};

#endif
