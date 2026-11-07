// DeviceEmu: the emulated IDirect3DDevice9. Set* calls record state (or go
// to the state block being recorded); _bindDeviceObjects applies what
// changed to GL before each draw.

#include "FHDeviceEmu.h"
#include "FHContext.h"
#include "FHRender.h"
#include <string.h>

LF_IMPLEMENT_CLASS(DeviceEmu, LF_CLASSOF(IUnknownEmu))

DeviceEmu *DeviceEmu::sCurrentDevice = 0;

DeviceEmu::DeviceEmu()
{
	_decl = 0;
	memset(_streams, 0, sizeof(_streams));
	memset(_strides, 0, sizeof(_strides));
	memset(_streamChanged, 0, sizeof(_streamChanged));
	_streamsChanged = false;
	_indices = 0;
	_indicesChanged = false;
	memset(_textures, 0, sizeof(_textures));
	memset(_textureChanged, 0, sizeof(_textureChanged));
	_vs = _ps = 0;
	_vsChanged = _psChanged = false;
	memset(_vsConsts, 0, sizeof(_vsConsts));
	memset(_vsConstDirty, 0, sizeof(_vsConstDirty));
	_vsDirtyCount = 0;
	memset(_psConsts, 0, sizeof(_psConsts));
	memset(_psConstDirty, 0, sizeof(_psConstDirty));
	_psDirtyCount = 0;
	_maxVSConsts = _maxPSConsts = 0;
	_recording = 0;
	memset(&_renderStates, 0, sizeof(_renderStates));
	_renderStatesChanged = false;
	memset(_textureStates, 0, sizeof(_textureStates));
	memset(_samplerStates, 0, sizeof(_samplerStates));
	memset(_savedMipFilter, 0, sizeof(_savedMipFilter));
	memset(_mipFilterApplied, 0, sizeof(_mipFilterApplied));
	_context = 0;
	_maxSimultTextures = 0;
	FHMatrixIdentity(&_world);
	FHMatrixIdentity(&_view);
	FHMatrixIdentity(&_projection);
	for (int i = 0; i < kMaxTextures; i++)
		FHMatrixIdentity(&_texture[i]);
	_primMode = GL_TRIANGLES;
	_indexType = 0;
	_indexBase = 0;
	_indexSize = 2;
	_vertexArrayOn = _normalArrayOn = _colorArrayOn = _secondaryColorArrayOn = _fogCoordArrayOn = false;
	_psizeAttribOn = _weightArrayOn = _blendIndicesAttribOn = _tangentAttribOn = _binormalAttribOn = false;
	memset(_texCoordArrayOn, 0, sizeof(_texCoordArrayOn));
}

DeviceEmu *DeviceEmu::initWithParams(CContext *context)
{
	if (LObject::init() == 0)
		return 0;
	_maxSimultTextures = 0;
	_maxVSConsts = 0;
	_maxPSConsts = 0;
	_context = context;
	return this;
}

void DeviceEmu::dealloc()
{
	for (int i = 0; i < kMaxStreams; i++)
		FHSafeRelease(_streams[i]);
	FHSafeRelease(_indices);
	for (int i = 0; i < kMaxTextures; i++)
		FHSafeRelease(_textures[i]);
	FHSafeRelease(_decl);
	FHSafeRelease(_vs);
	FHSafeRelease(_ps);
	FHSafeRelease(_recording);
	if (sCurrentDevice == this)
		sCurrentDevice = 0;
	LObject::dealloc();
}

bool DeviceEmu::ValidateContext()
{
	bool ok = _context->holder()->setCurrentGLContext();
	if (ok)
		sCurrentDevice = this;
	return ok;
}

CContextCaps *DeviceEmu::contextCaps() const
{
	return _context->caps();
}

void DeviceEmu::_vbWasChanged(VertexBufferEmu *vb)
{
	for (int i = 0; i < kMaxStreams; i++) {
		if (_streams[i] == vb) {
			_streamsChanged = true;
			_streamChanged[i] = true;
		}
	}
}

void DeviceEmu::_resetCurrentBuffers()
{
	for (int i = 0; i < kMaxStreams; i++) {
		FHSafeRelease(_streams[i]);
		_strides[i] = 4;
		_streamChanged[i] = true;
		_streamsChanged = true;
	}
	FHSafeRelease(_indices);
	_indicesChanged = true;
	_indexBase = 0;
}

// ---------------------------------------------------------------------------
// Resources

HRESULT DeviceEmu::CreateTexture(unsigned int width, unsigned int height, unsigned int levels, unsigned int usage,
                                 D3DFORMAT format, D3DPOOL pool, Texture2DEmu **texture, void *)
{
	*texture = 0;
	*texture = (Texture2DEmu *)Texture2DEmu::_alloc()->initWithParams(GL_TEXTURE_2D, width, height, format, levels,
	                                                                  usage, pool, this);
	return D3D_OK;
}

HRESULT DeviceEmu::CreateCubeTexture(unsigned int edge, unsigned int levels, unsigned int usage, D3DFORMAT format,
                                     D3DPOOL pool, CubicTextureEmu **texture, void *)
{
	*texture = (CubicTextureEmu *)CubicTextureEmu::_alloc()->initWithParams(GL_TEXTURE_CUBE_MAP, edge, edge, format,
	                                                                        levels, usage, pool, this);
	return *texture ? D3D_OK : D3DERR_OUTOFVIDEOMEMORY;
}

HRESULT DeviceEmu::CreateVertexBuffer(unsigned int length, unsigned int usage, unsigned int, D3DPOOL pool,
                                      VertexBufferEmu **vb, void *)
{
	VertexBufferEmu *b = VertexBufferEmu::_alloc()->initWithParams(length, usage, pool, this);
	if (b == 0)
		return D3DERR_OUTOFVIDEOMEMORY;
	*vb = b;
	return D3D_OK;
}

HRESULT DeviceEmu::CreateIndexBuffer(unsigned int length, unsigned int usage, D3DFORMAT format, D3DPOOL pool,
                                     IndexBufferEmu **ib, void *)
{
	IndexBufferEmu *b = IndexBufferEmu::_alloc()->initWithParams(format, length, usage, pool, this);
	if (b == 0)
		return D3DERR_OUTOFVIDEOMEMORY;
	*ib = b;
	return D3D_OK;
}

HRESULT DeviceEmu::CreateVertexDeclaration(const D3DVERTEXELEMENT9 *elements, VertexDeclEmu **decl)
{
	*decl = 0;
	VertexDeclEmu *d = VertexDeclEmu::_alloc();
	if (d == 0)
		return D3DERR_OUTOFVIDEOMEMORY;
	if (d->initWithParams(elements, this) == 0)
		return D3DERR_OUTOFVIDEOMEMORY;
	*decl = d;
	return D3D_OK;
}

// ---------------------------------------------------------------------------
// State

HRESULT DeviceEmu::SetRenderState(D3DRENDERSTATETYPE state, unsigned int value)
{
	if (_recording) {
		_recording->setRenderState(state, value);
	} else {
		_renderStates.s[state].changed = true;
		_renderStates.s[state].value = value;
		_renderStatesChanged = true;
	}
	return D3D_OK;
}

HRESULT DeviceEmu::SetTextureStageState(unsigned int stage, D3DTEXTURESTAGESTATETYPE type, unsigned int value)
{
	if (_recording) {
		_recording->setTextureStageState(stage, type, value);
	} else {
		_textureStates[stage].s[type].value = value;
		_textureStates[stage].s[type].changed = true;
	}
	return D3D_OK;
}

HRESULT DeviceEmu::SetSamplerState(unsigned int stage, D3DSAMPLERSTATETYPE type, unsigned int value)
{
	if (_recording) {
		_recording->setSamplerState(stage, type, value);
	} else {
		_samplerStates[stage].s[type].value = value;
		_samplerStates[stage].s[type].changed = true;
	}
	return D3D_OK;
}

HRESULT DeviceEmu::SetTexture(unsigned int stage, BaseTextureEmu *texture)
{
	if (_recording) {
		_recording->setTexture(stage, texture);
	} else if (_textures[stage] != texture) {
		FHAssign(_textures[stage], texture);
		_textureChanged[stage] = true;
	}
	return D3D_OK;
}

HRESULT DeviceEmu::SetVertexDeclaration(VertexDeclEmu *decl)
{
	if (_recording)
		_recording->setVertexDeclaration(decl);
	else
		FHAssign(_decl, decl);
	return D3D_OK;
}

HRESULT DeviceEmu::SetStreamSource(unsigned int stream, VertexBufferEmu *vb, unsigned int offset, unsigned int stride)
{
	if (_recording) {
		_recording->setStreamSource(stream, vb, offset, stride);
	} else if (_streams[stream] != vb) {
		FHAssign(_streams[stream], vb);
		_streamChanged[stream] = true;
		_streamsChanged = true;
		_strides[stream] = stride;
	}
	return D3D_OK;
}

HRESULT DeviceEmu::SetIndices(IndexBufferEmu *ib)
{
	if (_recording) {
		_recording->setIndices(ib);
	} else if (_indices != ib) {
		FHAssign(_indices, ib);
		_indicesChanged = true;
		_indexBase = 0;
	}
	return D3D_OK;
}

HRESULT DeviceEmu::SetTransform(D3DTRANSFORMSTATETYPE type, const _SMatrix *m)
{
	if (_recording) {
		_recording->setTransform(type, m);
		return D3D_OK;
	}
	_SMatrix *dst;
	if (type == D3DTS_VIEW)
		dst = &_view;
	else if (type == D3DTS_PROJECTION)
		dst = &_projection;
	else if (type >= D3DTS_TEXTURE0 && type <= D3DTS_TEXTURE7)
		dst = &_texture[type - D3DTS_TEXTURE0];
	else if (type == D3DTS_WORLD)
		dst = &_world;
	else
		return D3D_OK;
	*dst = *m;
	return D3D_OK;
}

HRESULT DeviceEmu::SetVertexShader(DeviceShaderEmu *shader)
{
	if (_recording) {
		_recording->setVertexShader(shader);
	} else if (_vs != shader) {
		FHAssign(_vs, shader);
		_vsChanged = true;
	}
	return D3D_OK;
}

HRESULT DeviceEmu::SetVertexShaderConstantF(unsigned int start, const float *data, unsigned int count)
{
	if (_recording) {
		_recording->setVertexShaderConstantF(start, data, count);
		return D3D_OK;
	}
	memcpy(_vsConsts[start], data, count * 16);
	for (unsigned int i = start; i < start + count; i++) {
		if (!_vsConstDirty[i]) {
			_vsConstDirty[i] = true;
			_vsDirtyList[_vsDirtyCount++] = i;
		}
	}
	return D3D_OK;
}

HRESULT DeviceEmu::SetPixelShader(DeviceShaderEmu *shader)
{
	if (_recording) {
		_recording->setPixelShader(shader);
	} else if (_ps != shader) {
		FHAssign(_ps, shader);
		_psChanged = true;
	}
	return D3D_OK;
}

HRESULT DeviceEmu::SetPixelShaderConstantF(unsigned int start, const float *data, unsigned int count)
{
	if (_recording) {
		_recording->setPixelShaderConstantF(start, data, count);
		return D3D_OK;
	}
	memcpy(_psConsts[start], data, count * 16);
	for (unsigned int i = start; i < start + count; i++) {
		if (!_psConstDirty[i]) {
			_psConstDirty[i] = true;
			_psDirtyList[_psDirtyCount++] = i;
		}
	}
	return D3D_OK;
}

HRESULT DeviceEmu::BeginStateBlock()
{
	if (_recording)
		return D3DERR_INVALIDCALL;
	StateBlockEmu *block = StateBlockEmu::_alloc()->initWithParams(this);
	if (block == 0)
		return D3DERR_OUTOFVIDEOMEMORY;
	_recording = block;
	return D3D_OK;
}

HRESULT DeviceEmu::EndStateBlock(StateBlockEmu **block)
{
	if (_recording == 0)
		return D3DERR_INVALIDCALL;
	*block = _recording;
	_recording = 0;
	return D3D_OK;
}

// ---------------------------------------------------------------------------
// Drawing

HRESULT DeviceEmu::_bindDeviceObjects(D3DPRIMITIVETYPE type)
{
	CContextCaps *caps = contextCaps();
	if (_maxSimultTextures == 0)
		_maxSimultTextures = caps->GetMaxSimultTextures();
	if (_maxVSConsts == 0)
		_maxVSConsts = caps->GetMaxVSConstantIndex() < (unsigned)kMaxVSConsts ? caps->GetMaxVSConstantIndex()
		                                                                      : (unsigned)kMaxVSConsts;
	if (_maxPSConsts == 0)
		_maxPSConsts = caps->GetMaxPSConstantIndex() < (unsigned)kMaxPSConsts ? caps->GetMaxPSConstantIndex()
		                                                                      : (unsigned)kMaxPSConsts;
	// The texture factor, before applying clears its flag: stages without a
	// constant of their own get it.
	unsigned int tfactor = _renderStates.s[D3DRS_TEXTUREFACTOR].value;
	bool tfactorChanged = _renderStates.s[D3DRS_TEXTUREFACTOR].changed;
	if (_renderStatesChanged) {
		DeviceStatesRemap::applyRenderStates(&_renderStates);
		_renderStatesChanged = false;
	}

	switch (type) {
	case D3DPT_POINTLIST:
		_primMode = GL_POINTS;
		break;
	case D3DPT_LINELIST:
		_primMode = GL_LINES;
		break;
	case D3DPT_LINESTRIP:
		_primMode = GL_LINE_STRIP;
		break;
	case D3DPT_TRIANGLELIST:
		_primMode = GL_TRIANGLES;
		break;
	case D3DPT_TRIANGLESTRIP:
		_primMode = GL_TRIANGLE_STRIP;
		break;
	case D3DPT_TRIANGLEFAN:
		_primMode = GL_TRIANGLE_FAN;
		break;
	default:
		return D3DERR_INVALIDCALL;
	}

	if (_indicesChanged) {
		if (_indices == 0) {
			_indexType = 0;
			_indexSize = 2;
			_indexBase = 0;
		} else {
			D3DINDEXBUFFER_DESC desc;
			_indices->GetDesc(&desc);
			if (desc.Format == D3DFMT_INDEX16) {
				_indexType = GL_UNSIGNED_SHORT;
				_indexSize = 2;
			} else if (desc.Format == D3DFMT_INDEX32) {
				_indexType = GL_UNSIGNED_INT;
				_indexSize = 4;
			} else {
				return D3DERR_INVALIDCALL;
			}
			_indexBase = _indices->bind();
		}
		_indicesChanged = false;
	}

	if (_decl == 0)
		return D3DERR_INVALIDCALL;

	if (_streamsChanged) {
		// Arrays still marked "off" afterwards were on and aren't used now.
		bool vertexOff = _vertexArrayOn, normalOff = _normalArrayOn, colorOff = _colorArrayOn,
		     secondaryOff = _secondaryColorArrayOn, fogOff = _fogCoordArrayOn, psizeOff = _psizeAttribOn,
		     weightOff = _weightArrayOn, blendIndicesOff = _blendIndicesAttribOn, tangentOff = _tangentAttribOn,
		     binormalOff = _binormalAttribOn;
		bool texCoordOff[kMaxTextures];
		memcpy(texCoordOff, _texCoordArrayOn, sizeof(texCoordOff));

		const D3DVERTEXELEMENT9 *e = _decl->vertexDeclaration();
		VertexBufferEmu *bound = 0;
		char *base = 0;
		for (int n = 0; n <= MAXD3DDECLLENGTH && e->Type != D3DDECLTYPE_UNUSED; n++, e++) {
			unsigned int stream = e->Stream;
			VertexBufferEmu *vb = _streams[stream];
			if (vb == 0)
				return D3DERR_INVALIDCALL;
			if (bound != vb) {
				if (bound)
					bound->unbind();
				base = vb->bind();
				bound = vb;
			}
			GLenum glType = GL_FLOAT;
			GLint count = 3;
			FHGetGLParamsFromVertexDeclType(e->Type, &glType, &count);
			GLsizei stride = _strides[stream];
			const char *ptr = base + e->Offset;
			switch (e->Usage) {
			case D3DDECLUSAGE_POSITION:
			case D3DDECLUSAGE_POSITIONT:
				if (!vertexOff) {
					glEnableClientState(GL_VERTEX_ARRAY);
					_vertexArrayOn = true;
				}
				glVertexPointer(count, glType, stride, ptr);
				vertexOff = false;
				break;
			case D3DDECLUSAGE_BLENDWEIGHT:
				// (The original tested this flag the wrong way round.)
				if (!weightOff) {
					glEnableClientState(GL_WEIGHT_ARRAY_ARB);
					_weightArrayOn = true;
				}
				glWeightPointerARB(count, glType, stride, (GLvoid *)ptr);
				weightOff = false;
				break;
			case D3DDECLUSAGE_BLENDINDICES:
				if (!blendIndicesOff) {
					glEnableVertexAttribArrayARB(7);
					_blendIndicesAttribOn = true;
				}
				glVertexAttribPointerARB(7, count, glType, GL_FALSE, stride, ptr);
				blendIndicesOff = false;
				break;
			case D3DDECLUSAGE_NORMAL:
				if (!normalOff) {
					glEnableClientState(GL_NORMAL_ARRAY);
					_normalArrayOn = true;
				}
				glNormalPointer(glType, stride, ptr);
				normalOff = false;
				break;
			case D3DDECLUSAGE_PSIZE:
				if (!psizeOff) {
					glEnableVertexAttribArrayARB(6);
					_psizeAttribOn = true;
				}
				glVertexAttribPointerARB(6, count, glType, GL_FALSE, stride, ptr);
				psizeOff = false;
				break;
			case D3DDECLUSAGE_TEXCOORD: {
				unsigned int unit = e->UsageIndex;
				glClientActiveTextureARB(GL_TEXTURE0_ARB + unit);
				if (!texCoordOff[unit]) {
					glEnableClientState(GL_TEXTURE_COORD_ARRAY);
					_texCoordArrayOn[unit] = true;
				} else {
					texCoordOff[unit] = false;
				}
				glTexCoordPointer(count, glType, stride, ptr);
				break;
			}
			case D3DDECLUSAGE_TANGENT:
				if (!tangentOff) {
					glEnableVertexAttribArrayARB(14);
					_tangentAttribOn = true;
				}
				glVertexAttribPointerARB(14, count, glType, GL_FALSE, stride, ptr);
				tangentOff = false;
				break;
			case D3DDECLUSAGE_BINORMAL:
				if (!binormalOff) {
					glEnableVertexAttribArrayARB(15);
					_binormalAttribOn = true;
				}
				glVertexAttribPointerARB(15, count, glType, GL_FALSE, stride, ptr);
				binormalOff = false;
				break;
			case D3DDECLUSAGE_COLOR:
				if (e->UsageIndex == 0) {
					if (!colorOff) {
						glEnableClientState(GL_COLOR_ARRAY);
						_colorArrayOn = true;
					}
					glColorPointer(count, glType, stride, ptr);
					colorOff = false;
				} else if (e->UsageIndex == 1) {
					if (!secondaryOff) {
						glEnableClientState(GL_SECONDARY_COLOR_ARRAY);
						_secondaryColorArrayOn = true;
					}
					glSecondaryColorPointerEXT(3, glType, stride, ptr);
					secondaryOff = false;
				}
				break;
			case D3DDECLUSAGE_FOG:
				if (!fogOff) {
					glEnableClientState(GL_FOG_COORDINATE_ARRAY);
					_fogCoordArrayOn = true;
				}
				glFogCoordPointer(glType, stride, ptr);
				fogOff = false;
				break;
			default:
				return D3DERR_INVALIDCALL;
			}
		}
		if (bound)
			bound->unbind();
		if (vertexOff) {
			glDisableClientState(GL_VERTEX_ARRAY);
			_vertexArrayOn = false;
		}
		if (normalOff) {
			glDisableClientState(GL_NORMAL_ARRAY);
			_normalArrayOn = false;
		}
		if (colorOff) {
			glDisableClientState(GL_COLOR_ARRAY);
			_colorArrayOn = false;
		}
		if (secondaryOff) {
			glDisableClientState(GL_SECONDARY_COLOR_ARRAY);
			_secondaryColorArrayOn = false;
		}
		if (fogOff) {
			glDisableClientState(GL_FOG_COORDINATE_ARRAY);
			_fogCoordArrayOn = false;
		}
		if (weightOff) {
			glDisableClientState(GL_WEIGHT_ARRAY_ARB);
			_weightArrayOn = false;
		}
		if (psizeOff) {
			glDisableVertexAttribArrayARB(6);
			_psizeAttribOn = false;
		}
		if (blendIndicesOff) {
			glDisableVertexAttribArrayARB(7);
			_blendIndicesAttribOn = false;
		}
		if (tangentOff) {
			glDisableVertexAttribArrayARB(14);
			_tangentAttribOn = false;
		}
		if (binormalOff) {
			glDisableVertexAttribArrayARB(15);
			_binormalAttribOn = false;
		}
		for (unsigned int i = 0; i < _maxSimultTextures && i < kMaxTextures; i++) {
			if (texCoordOff[i]) {
				glClientActiveTextureARB(GL_TEXTURE0_ARB + i);
				glDisableClientState(GL_TEXTURE_COORD_ARRAY);
				_texCoordArrayOn[i] = false;
			}
		}
		memset(_streamChanged, 0, sizeof(_streamChanged));
		_streamsChanged = false;
	}

	if (_vs == 0) {
		if (_vsChanged) {
			CVertexShader::unbindAllExcept(0);
			_vsChanged = false;
		}
	} else {
		GLenum target = _vs->target();
		if (_vsChanged) {
			_vs->bind();
			_vsChanged = false;
		}
		for (unsigned int i = 0; i < _vsDirtyCount; i++) {
			unsigned int c = _vsDirtyList[i];
			glProgramLocalParameter4fvARB(target, c, _vsConsts[c]);
			_vsConstDirty[c] = false;
		}
		_vsDirtyCount = 0;
	}
	if (_ps == 0) {
		if (_psChanged) {
			CPixelShader::unbindAllExcept(0);
			_psChanged = false;
		}
	} else {
		GLenum target = _ps->target();
		if (_psChanged) {
			_ps->bind();
			_psChanged = false;
		}
		for (unsigned int i = 0; i < _psDirtyCount; i++) {
			unsigned int c = _psDirtyList[i];
			glProgramLocalParameter4fvARB(target, c, _psConsts[c]);
			_psConstDirty[c] = false;
		}
		_psDirtyCount = 0;
	}

	for (unsigned int stage = 0; stage < _maxSimultTextures && stage < kMaxTextures; stage++) {
		if (!_textureChanged[stage])
			continue;
		BaseTextureEmu *texture = _textures[stage];
		if (texture == 0) {
			BaseTextureEmu::unbindAll(stage);
			continue;
		}
		texture->bind(stage);
		if (_ps == 0) {
			SStateValue &constant = _textureStates[stage].s[D3DTSS_CONSTANT];
			if (tfactorChanged && !constant.changed) {
				constant.value = tfactor;
				constant.changed = tfactorChanged;
			}
			DeviceStatesRemap::applyTextureStates(&_textureStates[stage], this);
		}
		// Textures without mip levels sample with the mip filter off; the
		// filter asked for is kept and restored for textures that have levels.
		SStateValue &mip = _samplerStates[stage].s[D3DSAMP_MIPFILTER];
		if (texture->GetLevelCount() < 2) {
			if (!mip.changed) {
				if (_mipFilterApplied[stage] && _savedMipFilter[stage] != D3DTEXF_NONE) {
					mip.value = D3DTEXF_NONE;
					mip.changed = true;
					_mipFilterApplied[stage] = false;
				}
			} else {
				_savedMipFilter[stage] = mip.value;
				if (mip.value == D3DTEXF_NONE) {
					_mipFilterApplied[stage] = true;
				} else {
					mip.value = D3DTEXF_NONE;
					_mipFilterApplied[stage] = false;
				}
			}
		} else if (!mip.changed) {
			if (!_mipFilterApplied[stage]) {
				mip.value = _savedMipFilter[stage];
				mip.changed = true;
				_mipFilterApplied[stage] = true;
			}
		} else {
			_savedMipFilter[stage] = mip.value;
			_mipFilterApplied[stage] = true;
		}
		DeviceStatesRemap::applySamplerStates(texture->target(), &_samplerStates[stage], this);
		if (_vs == 0) {
			// (The original multiplied onto the current texture matrix.)
			glMatrixMode(GL_TEXTURE);
			glLoadTransposeMatrixfARB(_texture[stage].m[0]);
		}
		_textureChanged[stage] = false;
	}

	if (_vs == 0) {
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(_projection.m[0]);
		_SMatrix worldView;
		FHMatrixMultiply(&worldView, &_world, &_view);
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf(worldView.m[0]);
	}
	return D3D_OK;
}

unsigned int calcIndicesCount(D3DPRIMITIVETYPE type, unsigned int primCount)
{
	switch (type) {
	case D3DPT_LINELIST:
		return primCount * 2;
	case D3DPT_LINESTRIP:
		return primCount + 1;
	case D3DPT_TRIANGLELIST:
		return primCount * 3;
	case D3DPT_TRIANGLESTRIP:
	case D3DPT_TRIANGLEFAN:
		return primCount + 2;
	default:
		return primCount;
	}
}

HRESULT DeviceEmu::DrawPrimitive(D3DPRIMITIVETYPE type, unsigned int startVertex, unsigned int primCount)
{
	HRESULT hr = _bindDeviceObjects(type);
	if (hr < 0)
		return hr;
	glDrawArrays(_primMode, startVertex, calcIndicesCount(type, primCount));
	return D3D_OK;
}

HRESULT DeviceEmu::DrawIndexedPrimitive(D3DPRIMITIVETYPE type, int, unsigned int, unsigned int,
                                        unsigned int startIndex, unsigned int primCount)
{
	HRESULT hr = _bindDeviceObjects(type);
	if (hr < 0)
		return hr;
	glDrawElements(_primMode, calcIndicesCount(type, primCount), _indexType, _indexBase + startIndex * _indexSize);
	return D3D_OK;
}
