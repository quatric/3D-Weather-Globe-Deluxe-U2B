// The flat map's coordinates grid.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(CFlatMapGridMarkers, LF_CLASSOF(CGridMarkers))

// Eight unit segments along x.
void CFlatMapGridMarkers::meshWasInstantiated(CMesh *mesh)
{
	{
		StVertexBufferLocker *locker = mesh->detachVBLocker(0, 0);
		char *p = (char *)locker->data();
		CRenderProfile *profile = mesh->renderProfile();
		int stride = profile->vertexStructSize(0);
		int position = profile->drawableElementOffset(D3DDECLUSAGE_POSITION, 0, 0);
		int index = profile->drawableElementOffset(D3DDECLUSAGE_TEXCOORD, 0, 0);
		for (int line = 0; line < 8; line++) {
			for (int end = 0; end < 2; end++, p += stride) {
				if (position >= 0) {
					float *v = (float *)(p + position);
					v[0] = end ? 1.0f : 0.0f;
					v[1] = 0.0f;
					v[2] = 0.0f;
				}
				if (index >= 0)
					*(float *)(p + index) = (float)line * 4.0f;
			}
		}
		delete locker;
	}
	StIndexBufferLocker *locker = mesh->detachIBLocker(0);
	short *indices = (short *)locker->data();
	for (short i = 0; i != 0x10; i += 2, indices += 2) {
		indices[0] = i;
		indices[1] = i + 1;
	}
	delete locker;
}

// Takes the next batch of lines, drawing the full one.
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

void CFlatMapGridMarkers::render(CFrameDrawer *drawer)
{
	SharedContextObjectsContainer *objects = _scene->objectsContainer();
	CHolder *holder = _scene->holder();
	LSize frame = holder->virtualFrameSize();
	float pixel = holder->fullFrameSize().width / holder->backBufferSize().width;
	Color color(0.3f, 0.75f, 1.0f, 1.0f);
	AtlasPanelController *controller = _scene->_controller;
	PanelDrawIterator *it = controller->detachDrawIterator(0);
	LPoint position = controller->position();
	const Matrix *viewProj = holder->context()->viewProjMatrix();
	CRenderTarget *target = holder->context()->currentRenderTarget();
	int cols, rows, segmentX, segmentY;
	target->getOutSegmentsCount(&cols, &rows);
	target->getOutSegment(&segmentX, &segmentY);

	int copy = 0;
	unsigned int count = 0;
	GridLineMeshData *data = 0;
	CFontDrawableData *font = 0;
	while (it->next()) {
		LRect mapRect;
		static_cast<FlatMapController::drawIteratorFlat *>(it)->evalRenderMapRect(position, &mapRect);

		// The parallels, captioned at the right of the last column.
		for (float latitude = _latitudeStep <= 11.0f ? -90.0f : -80.0f; latitude < 89.99f;
		     latitude += _latitudeStep) {
			LPoint geo = {position.x * 360.0f - 180.0f, latitude};
			LPoint view;
			_scene->viewPointFromGeoPoint(&view, &geo, it, 0);
			if (!(view.y > -10.0f && view.y < frame.height + 10.0f))
				continue;
			if (latitude > -89.99f) {
				data = nextLinesData(_gridMesh, data, &count, drawer);
				Matrix m;
				FHMatrixIdentity(&m);
				m._41 = it->_mapOffset;
				m._42 = (latitude + 90.0f) / 180.0f;
				Matrix wvp;
				FHMatrixMultiply(&wvp, &m, viewProj);
				FHMatrixTranspose(data->matrixForLineAtIndex(count), &wvp);
				count++;
			}
			if (copy == 0 && cols - 1 == segmentX) {
				view.x = frame.width - (_longitudeStep < 0.9999f ? 48.0f : 32.0f) * pixel;
				LString *caption = Utils::shortSingleCoordDescription(
				    latitude, 1, (Utils::TGeolocFormat)(_latitudeStep < 0.9999f));
				if (!font)
					font = objects->detachDataForSmallFont();
				font->addCaptionAtPosition(caption, view, &color);
			}
		}

		// The meridians, captioned at the bottom row.
		float longitude = -180.0f;
		do {
			LPoint geo = {longitude, position.y * 180.0f - 90.0f};
			LPoint view;
			_scene->viewPointFromGeoPoint(&view, &geo, it, 0);
			if (pixel * -35.0f <= view.x && view.x <= frame.width + pixel * 35.0f) {
				data = nextLinesData(_gridMesh, data, &count, drawer);
				Matrix m;
				FHMatrixIdentity(&m);
				m._11 = 0.0f;
				m._12 = 1.0f;
				m._21 = -1.0f;
				m._22 = 0.0f;
				m._41 = (longitude + 180.0f) / 360.0f + it->_mapOffset;
				Matrix wvp;
				FHMatrixMultiply(&wvp, &m, viewProj);
				FHMatrixTranspose(data->matrixForLineAtIndex(count), &wvp);
				count++;
				view.y = 0.0f;
				if (segmentY == 0) {
					if (!font)
						font = objects->detachDataForSmallFont();
					LString *caption = Utils::shortSingleCoordDescription(
					    longitude, 0, (Utils::TGeolocFormat)(_longitudeStep < 0.9999f));
					font->addCaptionAtPosition(caption, view, &color);
				}
			}
			longitude += _longitudeStep;
		} while (longitude < 179.99f);
		copy++;
	}
	if (font) {
		font->setDrawFlags(0);
		font->_storesState = false;
		drawer->renderDrawableData(font, 3);
	}
	if (count) {
		data->setLinesRenderCount(count);
		drawer->renderDrawableData(data, 3);
	}
	delete it;
}
