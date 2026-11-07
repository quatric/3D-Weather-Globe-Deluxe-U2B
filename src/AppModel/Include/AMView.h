// The globe's level-of-detail grid (SphereLOD) and terrain heights, and the
// route manager's flat preview map.

#ifndef AMVIEW_H
#define AMVIEW_H

#include "AMRoutes.h"


// A patch of the globe: column `x` of 2n, row `y` of n (from the south pole)
// at a level with n rows (1, 3, 9, 18, 54, 162 for levels 0-5). Also a tile of
// the route preview (column, row, level).
struct STextureID {
	// How many columns of the row are merged into one near the poles.
	static int columnsMergedInRow(int level, int y)
	{
		switch (level) {
		case 2:
			return (y == 0 || y == 8) ? 3 : 1;
		case 3:
			if ((unsigned)(y - 3) > 11) {
				int fromPole = y > 14 ? 17 - y : y;
				if (fromPole == 0)
					return 6;
				if (fromPole == 1)
					return 3;
				if (fromPole == 2)
					return 2;
			}
			return 1;
		case 4:
			return (unsigned)(y - 45) <= 2 ? 2 : 1;
		case 5:
			return (unsigned)(y - 135) <= 8 ? 2 : 1;
		default:
			return 1;
		}
	}

	// To the patch containing this one a level up; false at level 0.
	bool scaleDownIfPossible()
	{
		int l = level;
		if (l == 0)
			return false;
		x = (short)(x * columnsMergedInRow(l, y));
		level = (char)(l - 1);
		if (l == 3) {
			x /= 2;
			y /= 2;
		} else if (l < 6) {
			x /= 3;
			y /= 3;
		}
		x = (short)(x / columnsMergedInRow(l - 1, y));
		return true;
	}

	short x;
	short y;
	char level;
};

// Keys of a texture ID in the int-keyed sets and maps.
static inline intptr_t STextureIDKey(int x, int y, int level)
{
	return (x & 0xfff) | (y & 0xfff) << 12 | level << 24;
}

// The globe's patches: per level, the patch grid's face normals, bounding
// corners and index buffer offsets, the sines and cosines of the patch
// borders, and for the top level the patch rotation matrices.
class SphereLOD {
public:
	enum { kLevelsCount = 6, kMaxLevel = 5, kPatchesCount = 59150 };

	// The camera, from the globe center (radius 1), with its patch at each level.
	struct SCameraParams {
		SCameraParams(const Vector3 *position);

		float distance;	// from the center
		Vector3 position;
		Vector3 direction;	// normalized
		STextureID textureID;
		float longitude;	// 0..2pi
		float latitude;
		float cosLongitude;
		float sinLongitude;
		float cosLatitude;
		float sinLatitude;
		int columns[kLevelsCount];
		int rows[kLevelsCount];
		bool columnHalf[kLevelsCount];	// in the second half of the patch
		bool rowHalf[kLevelsCount];
	};

	SphereLOD() { _initTables(); }

	static void _initTables();
	static void entryRotationMatrix(const STextureID &id, Matrix *m);
	static bool doAllowHiResDrawForID(const STextureID &id);
	static float farClipDistanceForAltitude(float altitude);
	static float farClipValueForCameraParams(const Vector3 *camera, const Vector3 *direction, float altitude);
	static void entryTrapetionParams(const STextureID &id, Vector2 *params);
	static void getPatchIBParameters(const STextureID &id, int *count, int *offset);
	static const Vector3 *faceNormal(const STextureID &id);
	// The top-level patches facing a camera above `geoPoint` at `altitude`.
	static void fillVisibleEntries(LPoint geoPoint, float altitude, TArray<STextureID> *entries);
	// The distances from the camera to the nearest and the farthest point of
	// the patch, and their cosines to the horizon. False when the patch is
	// beyond the horizon.
	static bool getEntryViewParams(const STextureID &id, float height, const SCameraParams *camera, float *nearDistance,
	                               float *farDistance, float *nearCos, float *farCos);
	static bool getEntryViewParams(const STextureID &id, float height, const Vector3 *camera, float *nearDistance,
	                               float *farDistance, float *nearCos, float *farCos);

	static int _indexOf(const STextureID &id) { return id.x + (id.y * sItemsCountForLOD[(int)id.level] + sOffsetForLOD[(int)id.level]) * 2; }

	static int sOffsetForLOD[8];	// first patch of each level, in pairs
	static int sItemsCountForLOD[8];	// rows of each level
	static float *sSinArraysByLevel[kLevelsCount];	// 2n+1 border sines
	static float *sCosArraysByLevel[kLevelsCount];
	static Vector3 sFaceNormals[kPatchesCount];
	static Vector3 sMinCorners[kPatchesCount];
	static Vector3 sMaxCorners[kPatchesCount];
	static int sPatchIBOffsets[kPatchesCount];
	static Matrix sMaxLevelMatrices[162 * 324];
	static SphereLOD sInst;
};

// The terrain heights (the "Heights.zraw" resource: 16-bit samples, 0x8000
// for the sea level, twice as wide as high).
class HeightsManager {
public:
	HeightsManager() : _data(0), _samples(0), _height(0), _xScale(0), _width(0) {}
	~HeightsManager();

	static HeightsManager *manager();
	// The height in meters at a map point (0..1, y up), from the nearest sample.
	static int quickHeightForMapPoint(LPoint mapPoint);
	// The ground's distance from the center there, in Earth radii (heights exaggerated 3x).
	static float quickEarthRadiusFromMapPoint(LPoint mapPoint)
	{
		return quickHeightForMapPoint(mapPoint) * 4.7699996e-07f + 1.0f;
	}
	// The mean height of a top-level globe patch, in globe radii.
	static float zOffsetForElementWithID(const STextureID &id);
	void cleanup();

	LData *_data;
	const unsigned short *_samples;
	int _height;
	int _xScale;
	int _width;
	CHashTable _cache;	// unused
};

// ---------------------------------------------------------------------------
// The route preview: the route manager's map of the selected route, drawn from
// tiles of the "Earth preview" images (2x2, 4x4 or 8x8 of 256x128 by level).

// A route point in preview coordinates (0..1); `move` starts a new line,
// `hidden` points get no marker.
struct RPRoutePointData {
	LPoint point;
	bool move;
	bool hidden;
};

// A visible tile and the part of it shown.
struct RPTextureData {
	STextureID textureID;
	LRect srcRect;	// in the tile's pixels
};

// What to draw: the tiles, and the view x of each column and y of each row
// (ending with 1).
class RPDrawingData {
public:
	RPDrawingData();
	~RPDrawingData();
	void clean();

	TArray<RPTextureData> *_textures;
	TArray<float> *_columns;
	TArray<float> *_rows;
};

// The tile images of the last frames. The set holds the tiles loaded; the
// table counts the frames since a tile was dropped, the images go after 16.
class _RPTexturesCache : public CHashSet {
public:
	_RPTexturesCache();
	~_RPTexturesCache();

	LImage *imageForTextureID(const STextureID &id);
	void proceedNewFrame(CHashSet &textures);
	void _cacheTextures(CHashSet &textures);
	void _cleanUnusedCache();
	void _intersectSets(CHashSet &a, CHashSet &b, CHashSet &result);

	CHashTable _unused;	// tile key -> frames since it was dropped
	TLIntPtrMap<LImage *> *_images;
};

class RoutePreviewController : public LObject {
	LF_DECLARE_CLASS(RoutePreviewController)
public:
	static LString *kRoutesPreviewNeedsDisplay;

	RoutePreviewController();
	virtual RoutePreviewController *init();
	virtual void dealloc();
	virtual RoutePreviewController *getOwnerLObject() { return this; }

	bool isMinScaled();
	bool isMaxScaled();
	void setScale(float scale);
	void setPosition(LPoint position);
	void scrollWheelWithDiff(float diff);
	void mouseDraggedWithDiff(LPoint diff);
	void scaleToFit();
	void setSelectedRoute(GenericRouteNode *node);
	TArray<RPRoutePointData> *getRoutePointData();
	RPDrawingData *getDrawingData();
	LImage *imageForTextureID(const STextureID &id);

	int _lodForCurrentScale();
	int _numImagesForCurrentLOD();
	LSize _currentLODSize();
	float _fixedFloat(float value);
	LPoint _fixedPosition(LPoint position);
	LRect _renderRect();
	LPoint _renderPointFromViewPoint(LPoint point);
	LPoint _renderPointFromGeoPoint(LPoint geoPoint);
	TArray<RPRoutePointData> *_getRoutePointDataInLocalCoords();
	TArray<RPRoutePointData> *_routePointsToRenderViewPoints(TArray<RPRoutePointData> *points);
	LPoint _calculateSrcRectAndAnchorPointForPosition(int column, int row, LRect *srcRect);

	float _scale;	// of the shown part, 0.1..1
	LPoint _position;	// its center
	unsigned int _lod;
	RouteObject *_route;
	bool _routeChanged;
	bool _needsDisplay;
	RPDrawingData *_drawingData;
	TArray<RPRoutePointData> *_points;	// in map coordinates
	TArray<RPRoutePointData> *_viewPoints;
	_RPTexturesCache *_texturesCache;
};

#endif
