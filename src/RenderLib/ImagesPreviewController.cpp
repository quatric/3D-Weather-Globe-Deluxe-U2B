// ImagesPreviewController: the entry's photos as a cover flow over the
// scene, with PreviewBBMesh (the image quad and its fading reflection),
// PreviewDrawableData and the container's delegate.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(ImagesPreviewController, 0)
LF_IMPLEMENT_CLASS(PreviewBBMesh, LF_CLASSOF(CMesh))
LF_IMPLEMENT_CLASS(PreviewDrawableData, LF_CLASSOF(ModelDataWithMatrix))

ImagesPreviewController::ImagesPreviewController()
    : _delegate(0), _holder(0), _mesh(0), _textures(0), _selection(0), _scrollFrom(0), _scrollPosition(0),
      _scrollStart(-1.0)
{
	Vector3 eye(0, 0, 2), at(0, 0, 0), up(0, 1, 0);
	FHMatrixLookAtRH(&_view, &eye, &at, &up);
}

void ImagesPreviewController::dealloc()
{
	if (_mesh) {
		_mesh->dataSourceWillDie();
		releaseAndClear(_mesh);
	}
	releaseAndClear(_textures);
	LObject::dealloc();
}

ImagesPreviewController *ImagesPreviewController::initWithHolder(CHolder *holder, PreviewDelegate *delegate)
{
	if (!LObject::init())
		return 0;
	_holder = holder;
	_delegate = delegate;
	return this;
}

int ImagesPreviewController::imagesCount()
{
	return _delegate ? _delegate->imagesCountForPreview(this) : 10;
}

void ImagesPreviewController::dropCache()
{
	releaseAndClear(_textures);
}

// Two quads: the image (0..3) and its reflection (4..7), white fading from
// 0.35 to 0 down to 0.7 of the image's height.
void ImagesPreviewController::meshWasInstantiated(CMesh *mesh)
{
	CRenderProfile *profile = mesh->renderProfile();
	if (profile->_shader == 0)
		LF_RAISE(WrongConditionException());
	StVertexBufferLocker *vbLocker = mesh->detachVBLocker(0, 0);
	char *base = (char *)vbLocker->data();
	unsigned int stride = profile->vertexStructSize(0);
	int position = profile->drawableElementOffset(D3DDECLUSAGE_POSITION, 0, 0);
	int color = profile->drawableElementOffset(D3DDECLUSAGE_COLOR, 0, 0);
	int uv = profile->drawableElementOffset(D3DDECLUSAGE_TEXCOORD, 0, 0);
	static const float kPositions[8][2] = { { -0.5f, -0.5f }, { 0.5f, -0.5f }, { -0.5f, 0.5f }, { 0.5f, 0.5f },
		                                { -0.5f, -0.5f }, { 0.5f, -0.5f }, { -0.5f, -0.7f }, { 0.5f, -0.7f } };
	static const float kUVs[8][2] = { { 0, 1 }, { 1, 1 }, { 0, 0 }, { 1, 0 },
		                          { 0, 1 }, { 1, 1 }, { 0, 0.8f }, { 1, 0.8f } };
	static const float kAlphas[8] = { 1, 1, 1, 1, 0.35f, 0.35f, 0, 0 };
	for (int i = 0; i < 8; i++) {
		char *v = base + i * stride;
		float *p = (float *)(v + position);
		p[0] = kPositions[i][0];
		p[1] = kPositions[i][1];
		p[2] = 0;
		if (color >= 0)
			*(unsigned int *)(v + color) = Color(1, 1, 1, kAlphas[i]);
		if (uv >= 0) {
			float *t = (float *)(v + uv);
			t[0] = kUVs[i][0];
			t[1] = kUVs[i][1];
		}
	}
	delete vbLocker;
	StIndexBufferLocker *ibLocker = mesh->detachIBLocker(0);
	uint16_t *index = (uint16_t *)ibLocker->data();
	static const uint16_t kIndices[12] = { 0, 3, 1, 0, 2, 3, 4, 5, 6, 5, 7, 6 };
	memcpy(index, kIndices, sizeof(kIndices));
	delete ibLocker;
}

void ImagesPreviewController::scrollToImageAtIndex(int index)
{
	int count = imagesCount();
	if (index >= count)
		index = count - 1;
	if (index < 0)
		index = 0;
	if (index != _selection) {
		_selection = index;
		_scrollFrom = _scrollPosition;
		_scrollStart = LTimer::currentTimeValue();
	}
}

C2DTexture *ImagesPreviewController::_textureAtIndex(int index)
{
	if (_textures == 0) {
		_textures = LArray::_alloc()->init();
		int count = imagesCount();
		for (int i = 0; i < count; i++)
			_textures->addObject(LNull::null);
	}
	if (index < 0 || index >= (int)_textures->count())
		return 0;
	LObject *texture = _textures->objectAtIndex(index);
	if (texture != LNull::null)
		return (C2DTexture *)texture;
	if (_delegate == 0)
		return 0;
	C2DTexture *loaded = C2DTexture::textureWithContentsOfFile(_delegate->imageTexturePathForPreview(index, this), _holder);
	if (loaded == 0)
		return 0;
	_textures->replaceObjectAtIndex(loaded, index);
	return loaded;
}

PreviewDrawableData *ImagesPreviewController::_detachBBData()
{
	if (_mesh == 0) {
		CHolder *holder = _holder;
		if (holder)
			holder->beginAccumulationBlock();
		CRenderProfile *profile =
		    CRenderProfile::renderProfileByFileDesc(LString::stringWithCString("RENDERPROFILES\\ImagePreview.yaf"), _holder);
		_mesh = (PreviewBBMesh *)PreviewBBMesh::_alloc()->initWithDataSource(this, profile, 0);
		_mesh->setPerformBoxVisibilityTest(0);
		C2DTexture *border = C2DTexture::textureWithContentsOfFile(
		    LString::stringWithCString("TEXTURES\\Backgrounds\\border_512.dds"), _holder);
		_mesh->setTextureForSource(border, 1);
		if (holder)
			holder->endAccumulationBlock();
		if (_mesh == 0)
			return 0;
	}
	CDrawableModelData *data = _mesh->detachModelDataInstance();
	return data && data->getClass()->isSubclassOf(LF_CLASSOF(PreviewDrawableData)) ? (PreviewDrawableData *)data : 0;
}

// The images from the outside in, so nearer ones draw over farther ones:
// each turned and moved aside by its distance from the scroll position,
// fading out towards the ends.
void ImagesPreviewController::render(CFrameDrawer *drawer, float alpha)
{
	int count = 10;
	if (_delegate && (count = _delegate->imagesCountForPreview(this)) == 0)
		return;
	if (_selection < 0)
		_selection = 0;
	else if (_selection > count - 1)
		_selection = count - 1;
	double now = LTimer::currentTimeValue();
	int center = _selection;
	float position = (float)center;
	float fraction = 0;
	if (_scrollStart > 0.0) {
		float t = (float)((now - _scrollStart) / 0.4000000059604645);
		if (t <= 1.0f) {
			position = (position - _scrollFrom) * (float)pow((double)t, 0.5) + _scrollFrom;
			center = (int)floorf(_scrollPosition + 0.5f);
			fraction = position - (float)center;
		} else {
			_scrollStart = -1.0;
		}
	}
	// Away from the image the position moves through, the borders show.
	float moving = fabsf(position - _scrollPosition) * 4.0f - 0.5f;
	float still = 1.0f;
	if (moving >= 0)
		still = moving <= 1.0f ? 1.0f - moving : 0.0f;
	_scrollPosition = position;
	LSize size = _holder->virtualFrameSize();
	Matrix projection;
	FHMatrixPerspectiveFovRH(&projection, 0.69813174f, size.width / size.height, 0.5f, 10.0f);
	_holder->context()->currentRenderTarget()->adjustProjectionMatrixToCurrentSegment(&projection, &projection);
	Matrix viewProjection;
	FHMatrixMultiply(&viewProjection, &_view, &projection);
	for (int distance = 6; distance >= 0; distance--) {
		for (int index = center + distance; index >= center - distance; index -= distance * 2) {
			if (index >= 0 && index < count) {
				float offset = (float)(index - center) - fraction;
				float far = offset / 5.0f;
				far = far >= -1.0f ? (far <= 1.0f ? far : 1.0f) : -1.0f;
				float near = offset >= -1.0f ? (offset <= 1.0f ? offset : 1.0f) : -1.0f;
				double d = (double)(1.0f - fabsf(far));
				float p1 = (float)(pow(d, (double)0.3f) * 1.5);
				float fade = (float)(((1.0 - d) * (double)p1 + (double)((float)((double)p1 + pow(d, 5.0)) * (1.0f - fabsf(far)))) * 0.5);
				if (fade >= 1.0f)
					fade = 1.0f;
				Matrix world;
				FHMatrixRotationY(&world, -(far * 0.4f + near));
				world._41 = far + near * 0.45f;
				world._43 = fabsf(near) * -0.9f;
				PreviewDrawableData *data = _detachBBData();
				if (data == 0)
					return;
				data->setTextureForSource(_textureAtIndex(index), 0);
				if (_delegate) {
					float aspect = _delegate->imageAspectForPreview(index, this);
					if (aspect < 1.0f) {
						world._11 *= aspect;
						world._12 *= aspect;
						world._13 *= aspect;
					} else if (aspect > 1.0f) {
						float inverse = 1.0f / aspect;
						world._21 *= inverse;
						world._22 *= inverse;
						world._23 *= inverse;
					}
				}
				float front = 1.0f - fabsf(near);
				data->_color = Color(1, 1, 1, fade * ((1.0f - front) + front * still) * alpha);
				FHMatrixMultiply(&data->_matrix, &world, &viewProjection);
				drawer->renderDrawableData(data, 4);
			}
			if (distance == 0)
				break;
		}
	}
}

// ---------------------------------------------------------------------------
// PreviewBBMesh, PreviewDrawableData

LClass *PreviewBBMesh::modelDataClass()
{
	return LF_CLASSOF(PreviewDrawableData);
}

const void *PreviewDrawableData::_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer)
{
	if (descr->id != 'HoPs')
		return CDrawableModelData::_dataPtrForConstant(descr, buffer);
	return &_color;
}

// ---------------------------------------------------------------------------
// _ImagesPreviewControllerDS

static EntryInfo *_imagesInfo(SharedContextObjectsContainer *owner)
{
	return owner->_imagesEntry ? owner->_imagesEntry->entryInfoObject() : 0;
}

int _ImagesPreviewControllerDS::imagesCountForPreview(ImagesPreviewController *)
{
	EntryInfo *info = _imagesInfo(_owner);
	return info ? info->itemsCountForKey(kEntryInfoImagesKey, (TEntryKeySet)0, (TEntryKeyAccessMode)0) : 0;
}

LString *_ImagesPreviewControllerDS::imageTexturePathForPreview(int index, ImagesPreviewController *)
{
	EntryInfo *info = _imagesInfo(_owner);
	if (info == 0)
		return 0;
	EntryInfoImageWrapper *image = lobj_cast<EntryInfoImageWrapper>(
	    info->itemForKeyAtIndex(kEntryInfoImagesKey, index, (TEntryKeySet)0, (TEntryKeyAccessMode)0));
	return image ? image->texturePath() : 0;
}

float _ImagesPreviewControllerDS::imageAspectForPreview(int index, ImagesPreviewController *)
{
	EntryInfo *info = _imagesInfo(_owner);
	if (info == 0)
		return 1;
	EntryInfoImageWrapper *image = lobj_cast<EntryInfoImageWrapper>(
	    info->itemForKeyAtIndex(kEntryInfoImagesKey, index, (TEntryKeySet)0, (TEntryKeyAccessMode)0));
	return image ? image->aspectRatio() : 1;
}
