// The globe's coordinates grid.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(CGlobeMapGridMarkers, LF_CLASSOF(CGridMarkers))

static const float kPi = 3.1415927f;

void CGlobeMapGridMarkers::calcRenderParams(const Vector3 *cameraPosition, float *latFrom, float *latTo,
                                            float *lonFrom, float *lonTo)
{
	float x = cameraPosition->x, y = cameraPosition->y, z = cameraPosition->z;
	float distance = sqrtf(x * x + y * y + z * z);
	if (distance <= 1.0f)
		distance = 1.0f;
	z /= distance;
	z = z > 1.0f ? 1.0f : z;
	if (!(z >= -1.0f))
		z = -1.0f;
	float cx = x / sqrtf(x * x + y * y);
	cx = cx > 1.0f ? 1.0f : cx;
	if (!(cx >= -1.0f))
		cx = -1.0f;
	float latitude = asinf(z);
	float longitude = (float)(acos((double)cx) / 3.1415927410125732);
	if (y < 0.0f)
		longitude = 2.0f - longitude;
	// Half the visible cap, in half turns.
	float half = (float)(asin((double)(1.0f / distance)) / -3.1415927410125732 + 0.5) * 0.7f;
	float from = latitude / kPi - half;
	float to = latitude / kPi + half;
	from = from >= -0.5f ? from : -0.5f;
	to = to <= 0.5f ? to : 0.5f;
	float lonHalf = half / sqrtf(1.0001f - z * z);
	lonHalf = lonHalf > 1.0f ? 1.0f : lonHalf;
	if (!(lonHalf >= -1.0f))
		lonHalf = -1.0f;
	if (latFrom)
		*latFrom = from * 180.0f;
	if (latTo)
		*latTo = to * 180.0f;
	if (lonFrom)
		*lonFrom = (longitude - lonHalf) * 180.0f;
	if (lonTo)
		*lonTo = (longitude + lonHalf) * 180.0f;
}

// Eight unit circles in the xy plane, 180 segments each.
void CGlobeMapGridMarkers::meshWasInstantiated(CMesh *mesh)
{
	{
		StVertexBufferLocker *locker = mesh->detachVBLocker(0, 0);
		char *p = (char *)locker->data();
		CRenderProfile *profile = mesh->renderProfile();
		int stride = profile->vertexStructSize(0);
		int position = profile->drawableElementOffset(D3DDECLUSAGE_POSITION, 0, 0);
		int index = profile->drawableElementOffset(D3DDECLUSAGE_TEXCOORD, 0, 0);
		for (int line = 0; line < 8; line++) {
			for (int i = 0; i != 0xb5; i++, p += stride) {
				float a = ((float)i / 180.0f) * 6.2831855f;
				float c = cosf(a);
				float s = sinf(a);
				if (position >= 0) {
					float *v = (float *)(p + position);
					v[2] = 0.0f;
					v[1] = s;
					v[0] = c;
				}
				if (index >= 0)
					*(float *)(p + index) = (float)line * 4.0f;
			}
		}
		delete locker;
	}
	StIndexBufferLocker *locker = mesh->detachIBLocker(0);
	short *indices = (short *)locker->data();
	for (short first = 0; first != 0x5a8; first += 0xb5, indices += 0x168) {
		short *q = indices;
		short v = first;
		for (int i = 0; i < 0xb4; i++, q += 2) {
			q[0] = v;
			v++;
			q[1] = v;
		}
	}
	delete locker;
}

static GridLineMeshData *nextLinesData(GridLineMesh *mesh, GridLineMeshData *data, unsigned int *count,
                                       CFrameDrawer *drawer)
{
	if (!data) {
		data = lobj_cast<GridLineMeshData>(mesh->detachModelDataInstance());
		data->_storesState = false;
	}
	if (*count > 7) {
		data->setLinesRenderCount(*count);
		drawer->renderDrawableData(data, 3);
		data = lobj_cast<GridLineMeshData>(mesh->detachModelDataInstance());
		data->_storesState = false;
		*count = 0;
	}
	return data;
}

void CGlobeMapGridMarkers::render(CFrameDrawer *drawer)
{
	float latFrom, latTo, lonFrom, lonTo;
	calcRenderParams(_scene->camera()->position(), &latFrom, &latTo, &lonFrom, &lonTo);
	AtlasPanelController *controller = _scene->_controller;
	LPoint geo = geoPointFromMapPoint(controller->position());
	// Where the captions go: the meridians' along a parallel near the
	// center, the parallels' along a meridian at the left.
	float captionLatitude = geo.y < -75.0f ? -75.0f : (geo.y > 75.0f ? 75.0f : geo.y);
	float captionLongitude = geo.x - 45.0f;
	if (captionLongitude < -180.0f)
		captionLongitude += 360.0f;
	LPoint left;
	LPoint leftNormalized = {0.1f, 0.5f};
	if (controller->canConvertNormalizedViewPointToGeoPoint(leftNormalized, &left))
		captionLongitude = left.x;

	GridLineMeshData *data = 0;
	SharedContextObjectsContainer *objects = _scene->objectsContainer();
	_scene->holder()->virtualFrameSize();
	Color color(0.3f, 0.75f, 1.0f, 1.0f);
	const Matrix *viewProj = _scene->holder()->context()->viewProjMatrix();

	// A meridian and the one opposite are the same circle: drawn once.
	float steps = 360.01f / _longitudeStep;
	unsigned int count360 = steps <= 0.0f ? 0 : (unsigned int)steps;
	char drawn[722];
	memset(drawn, 0, count360 + 1);
	CFontDrawableData *font = 0;
	unsigned int count = 0;
	float longitude = -180.0f;
	do {
		LPoint captionGeo = {longitude, captionLatitude};
		bool visible = (lonFrom <= longitude && longitude <= lonTo) ||
		               (lonFrom <= longitude + 360.0f && longitude + 360.0f <= lonTo) ||
		               (lonFrom <= longitude - 360.0f && longitude - 360.0f <= lonTo);
		if (visible) {
			int index = (int)((longitude + 180.01f) / _longitudeStep);
			if (!drawn[index]) {
				drawn[index] = 1;
				int opposite = longitude >= 0.001f ? -(int)(count360 >> 1) : (int)(count360 >> 1);
				drawn[opposite + index] = 1;
				data = nextLinesData(_gridMesh, data, &count, drawer);
				float a = (longitude * kPi) / 180.0f;
				Vector3 axis(cosf(a), sinf(a), 0.0f);
				Matrix turn, wvp;
				FHMatrixRotationAxis(&turn, &axis, 1.5707964f);
				FHMatrixMultiply(&wvp, &turn, viewProj);
				FHMatrixTranspose(data->matrixForLineAtIndex(count), &wvp);
				count++;
			}
		}
		LPoint view;
		if (_scene->viewPointFromGeoPoint(&view, &captionGeo, 0, 0)) {
			LString *caption =
			    Utils::shortSingleCoordDescription(longitude, 0, (Utils::TGeolocFormat)(_longitudeStep < 0.9999f));
			if (!font)
				font = objects->detachDataForSmallFont();
			font->addCaptionAtPosition(caption, view, &color);
		}
		longitude += _longitudeStep;
	} while (longitude < 179.99f);

	for (float latitude = _latitudeStep <= 11.0f ? -90.0f : -80.0f; latitude < 89.99f; latitude += _latitudeStep) {
		if (!(latFrom <= latitude && latitude <= latTo))
			continue;
		if (latitude > -89.99f) {
			data = nextLinesData(_gridMesh, data, &count, drawer);
			float a = (latitude * kPi) / 180.0f;
			float c = cosf(a);
			Matrix m;
			FHMatrixIdentity(&m);
			m._11 = c;
			m._22 = c;
			m._43 = sinf(a);
			Matrix wvp;
			FHMatrixMultiply(&wvp, &m, viewProj);
			FHMatrixTranspose(data->matrixForLineAtIndex(count), &wvp);
			count++;
		}
		LPoint captionGeo = {captionLongitude, latitude};
		LPoint view;
		if (_scene->viewPointFromGeoPoint(&view, &captionGeo, 0, 0)) {
			LString *caption =
			    Utils::shortSingleCoordDescription(latitude, 1, (Utils::TGeolocFormat)(_latitudeStep < 0.9999f));
			if (!font)
				font = objects->detachDataForSmallFont();
			font->addCaptionAtPosition(caption, view, &color);
		}
	}
	if (font) {
		font->setDrawFlags(0);
		font->_storesState = false;
		drawer->renderDrawableData(font, 3);
	}
	if (data && count) {
		data->setLinesRenderCount(count);
		drawer->renderDrawableData(data, 3);
	}
}
