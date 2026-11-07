// SphereDrawing: printing the globe. The overlays are projected with the
// scene's view-projection matrix; the stars and the planet names are in a
// space matrix scaled inside the camera's far plane, clipped to the space
// around the earth. Also the RenderLib bundle accessors.

#import <Cocoa/Cocoa.h>
#include "RenderLib.h"

LF_IMPLEMENT_CLASS(SphereDrawing, LF_CLASSOF(BaseDrawing))

static CFBundleRef _sRenderLibBundle = 0;

class _RenderLibBundleCleanup : public LCleanupTask {
public:
	virtual void performCleanup()
	{
		if (_sRenderLibBundle)
			CFRelease(_sRenderLibBundle);
	}
};
static _RenderLibBundleCleanup _sRenderLibBundleCleanup;

void setRenderLibModule(CFBundleRef bundle)
{
	if (_sRenderLibBundle)
		CFRelease(_sRenderLibBundle);
	_sRenderLibBundle = bundle;
	if (bundle)
		CFRetain(bundle);
}

CFBundleRef getRenderLibModule()
{
	if (!_sRenderLibBundle)
		setRenderLibModule(CFBundleGetBundleWithIdentifier(CFSTR("com.mackiev.3DWG2.RenderLib")));
	return _sRenderLibBundle;
}

static void _identity(Matrix *m)
{
	memset(m, 0, sizeof(*m));
	m->_11 = m->_22 = m->_33 = m->_44 = 1.0f;
}

SphereDrawing::SphereDrawing() : _earthClipPath(0), _spaceMatrixDirty(true), _spaceMatrixWithStars(false)
{
	_identity(&_spaceMatrix);
}

void SphereDrawing::dealloc()
{
	BaseDrawing::dealloc();
}

SphereDrawing *SphereDrawing::initWithFrameSizeScene(LSize size, BaseScene *scene, unsigned int flags)
{
	BaseDrawing::initWithFrameSizeScene(size, scene, flags);
	return this;
}

void SphereDrawing::_releasePlatformSpecific()
{
	[(NSBezierPath *)_earthClipPath release];
	_earthClipPath = 0;
	BaseDrawing::_releasePlatformSpecific();
}

const Matrix *SphereDrawing::_createSpaceMatrix(unsigned char withStarsRotation)
{
	if (_spaceMatrixDirty || withStarsRotation != _spaceMatrixWithStars) {
		_identity(&_spaceMatrix);
		if (withStarsRotation) {
			TimeController *time = (TimeController *)LF_CLASSOF(TimeController)->singleton();
			Quater rotation = starsRotation_EarthSystem(time->date());
			FHMatrixRotationQuaternion(&_spaceMatrix, &rotation);
		}
		_spaceMatrixDirty = false;
		_spaceMatrixWithStars = withStarsRotation;
		CContextCamera *camera = _scene->camera();
		const Vector3 *position = camera->position();
		float scale = camera->farClip() * 0.95f;
		_spaceMatrix._11 *= scale;
		_spaceMatrix._12 *= scale;
		_spaceMatrix._13 *= scale;
		_spaceMatrix._21 *= scale;
		_spaceMatrix._22 *= scale;
		_spaceMatrix._23 *= scale;
		_spaceMatrix._31 *= scale;
		_spaceMatrix._32 *= scale;
		_spaceMatrix._33 *= scale;
		_spaceMatrix._41 = position->x;
		_spaceMatrix._42 = position->y;
		_spaceMatrix._43 = position->z;
	}
	return &_spaceMatrix;
}

LPoint SphereDrawing::_viewPointFromWorldCoords(const Vector3 &v, unsigned char *visible)
{
	Vector4 p;
	FHVec3Transform(&p, &v, &_viewProj);
	float w = p.w < 0 ? -p.w : p.w;
	float x = p.x / w, y = p.y / w, z = p.z / w;
	if (visible)
		*visible = !(z < -1 || z > 1 || x < -1 || x > 1 || y < -1 || y > 1);
	return LMakePoint((x + 1) * 0.5f * _frameSize.width, (y + 1) * 0.5f * _frameSize.height);
}

LPoint SphereDrawing::_viewPointFromLocalCoords(const Vector3 &v, const Matrix &m, unsigned char *visible)
{
	Vector4 p;
	FHVec3Transform(&p, &v, &m);
	Vector3 world(p.x, p.y, p.z);
	return _viewPointFromWorldCoords(world, visible);
}

void SphereDrawing::_saveGraphicsState()
{
	[NSGraphicsContext saveGraphicsState];
}

void SphereDrawing::_restoreGraphicsState()
{
	[NSGraphicsContext restoreGraphicsState];
}

// The earth (radius 1 at the origin) seen from the camera at distance d:
// its outline is the circle where the tangents from the camera touch it,
// at sqrt(d^2 - 1) / d from the axis and (d^2 - 1) / d from the camera
// toward the center.
void SphereDrawing::_calculateEarthPoints(LPoint &center, LPoint &p0, LPoint &p1, LPoint &p2, LPoint &p3,
                                          float &cosine)
{
	CContextCamera *camera = _scene->camera();
	const Vector3 *up = camera->viewUpVector();
	Vector3 position = *camera->position();
	Vector3 direction;
	FHVec3Normalize(&direction, camera->viewDirection());
	Vector3 side;
	FHVec3Cross(&side, &position, up);
	FHVec3Normalize(&side, &side);
	float d = sqrtf(position.x * position.x + position.y * position.y + position.z * position.z);
	Vector3 axis;
	FHVec3Normalize(&axis, &position);
	float k = d * d - 1.0f;
	float across = (1.0f / d) * sqrtf(k);
	float along = k / d;
	Vector3 a(across * side.x - along * axis.x + position.x, across * side.y - along * axis.y + position.y,
	          across * side.z - along * axis.z + position.z);
	Vector3 cross;
	FHVec3Cross(&cross, &position, &side);
	FHVec3Normalize(&side, &cross);
	Vector3 b(across * side.x - along * axis.x + position.x, across * side.y - along * axis.y + position.y,
	          across * side.z - along * axis.z + position.z);
	// a and b reflected through the axis.
	float axis2 = axis.x * axis.x + axis.y * axis.y + axis.z * axis.z;
	float da = axis.x * a.x + axis.y * a.y + axis.z * a.z;
	Vector3 c(da * (axis.x + axis.x) - a.x * axis2, da * (axis.y + axis.y) - a.y * axis2,
	          da * (axis.z + axis.z) - a.z * axis2);
	float db = axis.x * b.x + axis.y * b.y + axis.z * b.z;
	Vector3 e((axis.x + axis.x) * db - b.x * axis2, (axis.y + axis.y) * db - b.y * axis2,
	          db * (axis.z + axis.z) - axis2 * b.z);
	Vector3 origin(0, 0, 0);
	center = _viewPointFromWorldCoords(origin, 0);
	p0 = _viewPointFromWorldCoords(a, 0);
	p1 = _viewPointFromWorldCoords(b, 0);
	p2 = _viewPointFromWorldCoords(c, 0);
	p3 = _viewPointFromWorldCoords(e, 0);
	cosine = axis.x * direction.x + axis.y * direction.y + axis.z * direction.z;
}

// The view rect less the earth's disk (even-odd), cached. The original
// passes the arc's asinf (radians) as degrees; kept.
void SphereDrawing::_applyEarthClipRegion()
{
	if (_earthClipPath == 0) {
		_earthClipPath = [[NSBezierPath bezierPath] retain];
		LPoint center = LZeroPoint, p0 = LZeroPoint, p1 = LZeroPoint, p2 = LZeroPoint, p3 = LZeroPoint;
		float cosine = 0;
		_calculateEarthPoints(center, p0, p1, p2, p3, cosine);
		NSBezierPath *path = (NSBezierPath *)_earthClipPath;
		if (_controller->scale() <= 0.32f || cosine <= -0.903f) {
			[path appendBezierPathWithOvalInRect:NSMakeRect(p0.x, p0.y, p2.x - p0.x, p3.y - p0.y)];
		} else {
			float dx = fabsf(center.x - p2.x);
			float dy = fabsf(center.y - p3.y);
			float angle = asinf(dx / sqrtf(dx * dx + dy * dy));
			[path appendBezierPathWithArcWithCenter:NSMakePoint(center.x, center.y)
			                                 radius:dy
			                             startAngle:angle
			                               endAngle:180.0f - angle];
		}
		[path appendBezierPathWithRect:NSMakeRect(_clipRect.origin.x, _clipRect.origin.y, _clipRect.size.width,
		                                          _clipRect.size.height)];
		[path setWindingRule:NSWindingRuleEvenOdd];
	}
	[(NSBezierPath *)_earthClipPath addClip];
}

// The meridians and the parallels as polylines of 721 points, each with
// its coordinate's caption.
void SphereDrawing::drawGridInRect(LRect)
{
	float latFrom, latTo, lonFrom, lonTo;
	CGlobeMapGridMarkers::calcRenderParams(_scene->camera()->position(), &latFrom, &latTo, &lonFrom, &lonTo);
	CPRFont *font = gridMarkerFont();
	float lonStep = degreesGridStep(1);
	float latStep = degreesGridStep(0);
	LPoint here = geoPointFromMapPoint(_controller->position());
	float labelLat = here.y < -75.0f ? -75.0f : here.y > 75.0f ? 75.0f : here.y;
	float labelLon = (1.05f - _controller->scale()) * -10.0f + here.x;
	if (labelLon < -180.0f)
		labelLon += 360.0f;
	LPoint left;
	if (_controller->canConvertNormalizedViewPointToGeoPoint(LMakePoint(0.2f, 0.5f), &left))
		labelLon = left.x;
	_setPrimaryColor(gridColor());
	_beginDrawingLines(0.5f);
	float steps = 360.01f / lonStep;
	unsigned int count = steps <= 0 ? 0 : (unsigned int)steps;
	char *drawn = (char *)calloc(count + 1 + count / 2 + 1, 1);
	unsigned char visible = 0;
	for (float lon = -180.0f; lon < 179.99f; lon += lonStep) {
		bool inView = (lonFrom <= lon && lon <= lonTo) || (lonFrom <= lon + 360.0f && lon + 360.0f <= lonTo) ||
		              (lonFrom <= lon - 360.0f && lon - 360.0f <= lonTo);
		int index = (int)((lon + 180.01f) / lonStep);
		if (inView && !drawn[index]) {
			// The meridian and its opposite are one great circle.
			drawn[index] = 1;
			int opposite = index + (lon < 0.001f ? (int)(count >> 1) : -(int)(count >> 1));
			if (opposite >= 0 && opposite <= (int)count)
				drawn[opposite] = 1;
			bool wasVisible = false;
			LPoint previous = LZeroPoint;
			for (int k = 0; k < 721; k++) {
				LPoint geo;
				if (k < 360)
					geo = LMakePoint(lon, (k / 720.0f) * 360.0f - 90.0f);
				else
					geo = LMakePoint(lon + 180.0f, (k / 720.0f) * -360.0f + 270.0f);
				LPoint p = viewPointFromGeoPoint(geo, 0, &visible);
				if (k != 0) {
					if ((!wasVisible || k == 1) && visible)
						_drawLine(previous, p);
					else if (wasVisible || visible)
						_drawLineTo(p);
				}
				wasVisible = visible;
				previous = p;
			}
		}
		LPoint at = viewPointFromGeoPoint(LMakePoint(lon, labelLat), 0, &visible);
		if (visible)
			_drawString(font, Utils::shortSingleCoordDescription(lon, 0, (Utils::TGeolocFormat)(lonStep < 0.9999f)),
			            at);
	}
	free(drawn);
	for (float lat = latStep <= 11.0f ? -90.0f : -80.0f; lat < 89.99f; lat += latStep) {
		if (lat < latFrom || lat > latTo)
			continue;
		if (lat > -89.99f) {
			bool wasVisible = false;
			LPoint previous = LZeroPoint;
			for (int k = 0; k < 721; k++) {
				LPoint p = viewPointFromGeoPoint(LMakePoint((k / 720.0f) * 360.0f, lat), 0, &visible);
				if (k != 0) {
					if ((!wasVisible || k == 1) && visible)
						_drawLine(previous, p);
					else if (wasVisible || visible)
						_drawLineTo(p);
				}
				wasVisible = visible;
				previous = p;
			}
		}
		LPoint at = viewPointFromGeoPoint(LMakePoint(labelLon, lat), 0, &visible);
		if (visible)
			_drawString(font, Utils::shortSingleCoordDescription(lat, 1, (Utils::TGeolocFormat)(latStep < 0.9999f)),
			            at);
	}
	_endDrawingLines();
}

void SphereDrawing::drawConstellationsInRect(LRect)
{
	_saveGraphicsState();
	if (_flags & kPrintFlagEarth)
		_applyEarthClipRegion();
	AtlasDatabase *db = (AtlasDatabase *)LF_CLASSOF(AtlasDatabase)->singleton();
	LData *stars = db->constellationsStarsArray();
	LData *lines = db->constellationsIndsArray();
	unsigned int count = lines ? (unsigned int)(lines->length() >> 2) : 0;
	const char *star = stars ? (const char *)stars->bytes() : 0;
	const uint16_t *index = lines ? (const uint16_t *)lines->bytes() : 0;
	const Matrix *space = _createSpaceMatrix(1);
	_setPrimaryColor(constellationsColor());
	_beginDrawingLines(0.35f);
	for (unsigned int i = 0; i < count; i++, index += 2) {
		// The star records are 16 bytes, the position first.
		Vector3 a = *(const Vector3 *)(star + index[0] * 16);
		Vector3 b = *(const Vector3 *)(star + index[1] * 16);
		unsigned char visibleA = 0, visibleB = 0;
		LPoint pa = _viewPointFromLocalCoords(a, *space, &visibleA);
		LPoint pb = _viewPointFromLocalCoords(b, *space, &visibleB);
		if (visibleA || visibleB)
			_drawLine(pa, pb);
	}
	_endDrawingLines();
	_restoreGraphicsState();
}

// A caption centered 7 points above a point.
static void _drawCenteredCaption(SphereDrawing *self, LString *text, LPoint at, CPRFont *font, CPRFont *shadow)
{
	at.y += 7.0f;
	LSize size = font->stringSize(text);
	at.x += size.width * -0.5f;
	at.y += size.height * -0.5f;
	self->_drawString(shadow, text, at);
	at.y += 0.5f;
	at.x -= 1.0f;
	self->_drawString(font, text, at);
}

void SphereDrawing::drawConstellationsNamesInRect(LRect)
{
	const Matrix *space = _createSpaceMatrix(1);
	_saveGraphicsState();
	if (_flags & kPrintFlagEarth)
		_applyEarthClipRegion();
	AtlasDatabase *db = (AtlasDatabase *)LF_CLASSOF(AtlasDatabase)->singleton();
	LMapIterator it(db->constellations(), 1);
	while (it.next()) {
		LString *name = (LString *)it.key();
		Vector3 position;
		((LValue *)it.object())->getValue(&position);
		unsigned char visible = 0;
		LPoint at = _viewPointFromLocalCoords(position, *space, &visible);
		if (visible && name)
			_drawCenteredCaption(this, name, at, constellationNameFont(), constellationNameShadowFont());
	}
	_restoreGraphicsState();
}

// The planets (without the earth) as seen from the earth.
void SphereDrawing::drawPlanetsNamesInRect(LRect)
{
	_saveGraphicsState();
	if (_flags & kPrintFlagEarth)
		_applyEarthClipRegion();
	TimeController *time = (TimeController *)LF_CLASSOF(TimeController)->singleton();
	Vector3 earth = *time->planetPosition(UPlanets::kEarth);
	Vector3 directions[8];
	for (int i = 0; i < 8; i = i + 1 == UPlanets::kEarth ? i + 2 : i + 1) {
		Vector3 v = *time->planetPosition((UPlanets::TPlanet)i) - earth;
		Vector3 local = earthVecFromEclipticVec(&v, time->date());
		FHVec3Normalize(&directions[i], &local);
	}
	const Matrix *space = _createSpaceMatrix(0);
	for (int i = 0; i < 8; i = i + 1 == UPlanets::kEarth ? i + 2 : i + 1) {
		unsigned char visible = 0;
		LPoint at = _viewPointFromLocalCoords(directions[i], *space, &visible);
		if (!visible)
			continue;
		LString *name = LString::stringFromTable(Stars::planetName(i), "RenderLib", getRenderLibModule());
		if (name)
			_drawCenteredCaption(this, name, at, planetNameFont(), planetNameShadowFont());
	}
	_restoreGraphicsState();
}
