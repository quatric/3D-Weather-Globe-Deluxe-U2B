// CSceneControl: a button drawn over the scene.

#include "FHSprites.h"

LF_IMPLEMENT_CLASS(CSceneControl, 0)

LConstantString kControlBillboardModelFHKey("kControlBillboardModelFHKey");

CSceneControl::CSceneControl()
    : _holder(0), _scene(0), _title(0), _normalTexture(0), _overTexture(0), _altTexture(0), _modelData(0),
      _altIsNormal(false), _useNrmPos(true), _nrmPos(LZeroPoint), _ptOff(LZeroPoint), _viewRect(LZeroRect), _state(0),
      _tag(0), _target(0), _selector(0), _isToggle(false), _value(false), _transparency(1), _normalPath(0),
      _overPath(0), _altPath(0), _tracking(false), _inside(false)
{
}

void CSceneControl::dealloc()
{
	LRelease(_title);
	_title = 0;
	LRelease(_normalTexture);
	_normalTexture = 0;
	LRelease(_overTexture);
	_overTexture = 0;
	LRelease(_altTexture);
	_altTexture = 0;
	LRelease(_normalPath);
	_normalPath = 0;
	LRelease(_overPath);
	_overPath = 0;
	LRelease(_altPath);
	_altPath = 0;
	LRelease(_modelData);
	_modelData = 0;
	LObject::dealloc();
}

static LString *_copyPath(LString *path)
{
	if (path == 0)
		return 0;
	LString *copy = LString::_alloc()->init();
	copy->setValue(path);
	return copy;
}

CSceneControl *CSceneControl::initWithPaths(LString *normal, LString *over, LString *alt, CHolder *holder)
{
	if (LObject::init() == 0)
		return 0;
	_holder = holder;
	_normalPath = _copyPath(normal);
	_overPath = _copyPath(over);
	_altPath = _copyPath(alt);
	return this;
}

void CSceneControl::setNrmPosWithPtOff(LPoint nrm, LPoint off)
{
	_nrmPos = nrm;
	_ptOff = off;
	if (_scene)
		_scene->invalControlsLayout();
	_useNrmPos = true;
}

void CSceneControl::setTitle(LString *title)
{
	LAssign(_title, title);
}

void CSceneControl::beginTracking()
{
	_tracking = true;
	_inside = true;
	_state = kControlStatePressed;
}

// A click is a release inside a visible control; toggles flip their value.
void CSceneControl::endTracking()
{
	_tracking = false;
	_state = _inside ? kControlStateOver : kControlStateNormal;
	if (_inside && _transparency > 0.01f) {
		if (_isToggle) {
			_state = kControlStateNormal;
			_value = !_value;
		}
		performAction();
	}
}

void CSceneControl::mouseEnter()
{
	_inside = true;
	_state = _tracking ? kControlStatePressed : kControlStateOver;
}

void CSceneControl::mouseLeave()
{
	_inside = false;
	_state = kControlStateNormal;
}

void CSceneControl::performAction()
{
	if (_target)
		_target->invokeSelector(_selector, this);
}

static C2DTexture *_loadTexture(CHolder *holder, LString *path)
{
	if (holder)
		holder->beginAccumulationBlock();
	C2DTexture *texture = C2DTexture::_alloc()->initWithHolderAndPath(holder, path);
	if (holder)
		holder->endAccumulationBlock();
	return texture;
}

C2DTexture *CSceneControl::normalTexture()
{
	if (_normalTexture == 0 && _normalPath)
		_normalTexture = _loadTexture(_holder, _normalPath);
	return _normalTexture;
}

C2DTexture *CSceneControl::overTexture()
{
	if (_overTexture == 0 && _overPath)
		_overTexture = _loadTexture(_holder, _overPath);
	return _overTexture;
}

// Without its own picture the alternate state shows the normal one dimmed.
C2DTexture *CSceneControl::altTexture()
{
	if (_altTexture == 0) {
		if (_altPath)
			_altTexture = _loadTexture(_holder, _altPath);
		if (_altTexture == 0) {
			_altIsNormal = true;
			_altTexture = LRetain(normalTexture());
		}
	}
	return _altTexture;
}

LSize CSceneControl::size()
{
	C2DTexture *texture = normalTexture();
	if (texture == 0)
		return LZeroSize;
	LSize size = {(float)texture->_info.Width, (float)texture->_info.Height};
	return size;
}

void CSceneControl::evalViewRectForViewFrameWithSize(LSize size)
{
	if (!_useNrmPos)
		return;
	LSize s = this->size();
	_viewRect.origin.x = size.width * _nrmPos.x + _ptOff.x + s.width * -0.5f;
	_viewRect.origin.y = size.height * _nrmPos.y + _ptOff.y + s.height * -0.5f;
	_viewRect.size = s;
}

// The controls share one billboard model per holder.
void CSceneControl::_createModelData()
{
	LObject *model = _holder->objectForKey(&kControlBillboardModelFHKey);
	if (model == 0) {
		CRenderProfile *profile = CRenderProfile::renderProfileByFileDesc(&BillboardRenderProfile, _holder);
		CHolder *holder = _holder;
		if (holder)
			holder->beginAccumulationBlock();
		model = CBillboardModel::_alloc()->initWithRenderProfile(profile);
		if (model) {
			_holder->setObjectForKey(model, &kControlBillboardModelFHKey);
			model->release();
		}
		if (holder)
			holder->endAccumulationBlock();
		if (model == 0)
			return;
	}
	if (model->getClass()->isSubclassOf(LF_CLASSOF(CBillboardModel))) {
		_modelData = (CBillboardModelData *)LRetain(((CBillboardModel *)model)->detachModelDataInstance());
		_modelData->_storesState = false;
	}
}

void CSceneControl::render(CFrameDrawer *drawer, CFontDrawableData *font)
{
	if (_modelData == 0) {
		_createModelData();
		if (_modelData == 0)
			return;
	}
	C2DTexture *texture = 0;
	if (_state == kControlStateOver)
		texture = overTexture();
	else if (_state != kControlStatePressed && _state != kControlStateNormal)
		return;
	if (texture == 0) {
		texture = _value ? altTexture() : normalTexture();
		if (texture == 0)
			return;
	}
	_modelData->setTextureForSource(texture, 0);
	LRect rect;
	rect.origin.x = floorf(_viewRect.origin.x);
	rect.origin.y = floorf(_viewRect.origin.y);
	rect.size.width = floorf(_viewRect.size.width);
	rect.size.height = floorf(_viewRect.size.height);
	CBillboardSingleData *billboard = _modelData->billboardAtIndex(0, 1);
	billboard->setScreenRect(rect);
	Color color;
	color.r = color.g = color.b = _state == kControlStatePressed ? 0.35f : _altIsNormal ? 0.5f : 1.0f;
	color.a = _transparency;
	billboard->setDiffuseColor(&color);
	drawer->renderDrawableData(_modelData, 4);
	if (font && _title) {
		LRect caption = rect;
		caption.origin.y = rect.origin.y + rect.size.height;
		caption.size.height = (float)((CFontDrawable *)font->_model)->_fontSize;
		font->addCaptionInRect(_title, caption, 0);
	}
}
