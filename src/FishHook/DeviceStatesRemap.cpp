// DeviceStatesRemap: D3D render, texture-stage and sampler states to GL.
// Only the states marked changed are applied; the flags are then cleared.
// Colours are packed with red in the low byte (as Color makes them).

#include "FHDeviceEmu.h"

static GLenum _cmpFunc(unsigned int f)
{
	return (f >= D3DCMP_LESS && f <= D3DCMP_ALWAYS) ? GL_NEVER + (f - 1) : GL_NEVER;
}

static GLenum _blendMode(unsigned int b)
{
	switch (b) {
	case D3DBLEND_ONE:
		return GL_ONE;
	case D3DBLEND_SRCCOLOR:
		return GL_SRC_COLOR;
	case D3DBLEND_INVSRCCOLOR:
		return GL_ONE_MINUS_SRC_COLOR;
	case D3DBLEND_SRCALPHA:
		return GL_SRC_ALPHA;
	case D3DBLEND_INVSRCALPHA:
		return GL_ONE_MINUS_SRC_ALPHA;
	case D3DBLEND_DESTALPHA:
		return GL_DST_ALPHA;
	case D3DBLEND_INVDESTALPHA:
		return GL_ONE_MINUS_DST_ALPHA;
	case D3DBLEND_DESTCOLOR:
		return GL_DST_COLOR;
	case D3DBLEND_INVDESTCOLOR:
		return GL_ONE_MINUS_DST_COLOR;
	case D3DBLEND_SRCALPHASAT:
		return GL_SRC_ALPHA_SATURATE;
	case D3DBLEND_BLENDFACTOR:
		return GL_CONSTANT_COLOR;
	case D3DBLEND_INVBLENDFACTOR:
		return GL_ONE_MINUS_CONSTANT_COLOR;
	default:
		return GL_ZERO;
	}
}

static GLenum _blendOp(unsigned int op)
{
	switch (op) {
	case D3DBLENDOP_SUBTRACT:
		return GL_FUNC_SUBTRACT;
	case D3DBLENDOP_REVSUBTRACT:
		return GL_FUNC_REVERSE_SUBTRACT;
	case D3DBLENDOP_MIN:
		return GL_MIN;
	case D3DBLENDOP_MAX:
		return GL_MAX;
	default:
		return GL_FUNC_ADD;
	}
}

static GLenum _stencilOp(unsigned int op)
{
	switch (op) {
	case D3DSTENCILOP_KEEP:
		return GL_KEEP;
	case D3DSTENCILOP_REPLACE:
		return GL_REPLACE;
	case D3DSTENCILOP_INCRSAT:
		return GL_INCR;
	case D3DSTENCILOP_DECRSAT:
		return GL_DECR;
	case D3DSTENCILOP_INVERT:
		return GL_INVERT;
	case D3DSTENCILOP_INCR:
		return GL_INCR_WRAP;
	case D3DSTENCILOP_DECR:
		return GL_DECR_WRAP;
	default:	// (ZERO too, as the original)
		return 0;
	}
}

static GLint _glTextureWrapMode(unsigned int mode)
{
	if (mode == D3DTADDRESS_CLAMP)
		return GL_CLAMP_TO_EDGE;
	if (mode == D3DTADDRESS_BORDER)
		return GL_CLAMP_TO_BORDER;
	return GL_REPEAT;
}

static void _colorToFloats(unsigned int c, float *f)
{
	f[0] = (float)(c & 0xff) / 255.0f;
	f[1] = (float)((c >> 8) & 0xff) / 255.0f;
	f[2] = (float)((c >> 16) & 0xff) / 255.0f;
	f[3] = (float)(c >> 24) / 255.0f;
}

static inline float _asFloat(unsigned int v)
{
	union {
		unsigned int i;
		float f;
	} u;
	u.i = v;
	return u.f;
}

static void _enableIf(GLenum cap, bool on)
{
	if (on)
		glEnable(cap);
	else
		glDisable(cap);
}

void DeviceStatesRemap::applyRenderStates(SRenderStates *rs)
{
	SStateValue *s = rs->s;
#define CHANGED(n) s[n].changed
#define VALUE(n) s[n].value
	if (CHANGED(D3DRS_ALPHATESTENABLE))
		_enableIf(GL_ALPHA_TEST, VALUE(D3DRS_ALPHATESTENABLE) != 0);
	if (CHANGED(D3DRS_DITHERENABLE))
		_enableIf(GL_DITHER, VALUE(D3DRS_DITHERENABLE) != 0);
	if (CHANGED(D3DRS_ALPHABLENDENABLE))
		_enableIf(GL_BLEND, VALUE(D3DRS_ALPHABLENDENABLE) != 0);
	if (CHANGED(D3DRS_STENCILENABLE))
		_enableIf(GL_STENCIL_TEST, VALUE(D3DRS_STENCILENABLE) != 0);
	if (CHANGED(D3DRS_LIGHTING))
		_enableIf(GL_LIGHTING, VALUE(D3DRS_LIGHTING) != 0);
	if (CHANGED(D3DRS_NORMALIZENORMALS))
		_enableIf(GL_NORMALIZE, VALUE(D3DRS_NORMALIZENORMALS) != 0);
	if (CHANGED(D3DRS_SCISSORTESTENABLE))
		_enableIf(GL_SCISSOR_TEST, VALUE(D3DRS_SCISSORTESTENABLE) != 0);
	if (CHANGED(D3DRS_ANTIALIASEDLINEENABLE))
		_enableIf(GL_LINE_SMOOTH, VALUE(D3DRS_ANTIALIASEDLINEENABLE) != 0);
	if (CHANGED(D3DRS_ZWRITEENABLE))
		glDepthMask(VALUE(D3DRS_ZWRITEENABLE) != 0);
	if (CHANGED(D3DRS_FOGSTART))
		glFogf(GL_FOG_START, _asFloat(VALUE(D3DRS_FOGSTART)));
	if (CHANGED(D3DRS_FOGEND))
		glFogf(GL_FOG_END, _asFloat(VALUE(D3DRS_FOGEND)));
	if (CHANGED(D3DRS_FOGDENSITY))
		glFogf(GL_FOG_DENSITY, _asFloat(VALUE(D3DRS_FOGDENSITY)));
	if (CHANGED(D3DRS_STENCILWRITEMASK))
		glStencilMask(VALUE(D3DRS_STENCILWRITEMASK));
	if (CHANGED(D3DRS_POINTSIZE))
		glPointSize(_asFloat(VALUE(D3DRS_POINTSIZE)));
	if (CHANGED(D3DRS_BLENDOP))
		glBlendEquation(_blendOp(VALUE(D3DRS_BLENDOP)));
	if (CHANGED(D3DRS_ZFUNC))
		glDepthFunc(_cmpFunc(VALUE(D3DRS_ZFUNC)));
	if (CHANGED(D3DRS_ZENABLE)) {
		if (VALUE(D3DRS_ZENABLE) == D3DZB_FALSE)
			glDisable(GL_DEPTH_TEST);
		else if (VALUE(D3DRS_ZENABLE) == D3DZB_TRUE)
			glEnable(GL_DEPTH_TEST);
	}
	if (CHANGED(D3DRS_FILLMODE)) {
		switch (VALUE(D3DRS_FILLMODE)) {
		case D3DFILL_POINT:
			glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
			break;
		case D3DFILL_WIREFRAME:
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			break;
		case D3DFILL_SOLID:
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			break;
		}
	}
	if (CHANGED(D3DRS_SHADEMODE)) {
		if (VALUE(D3DRS_SHADEMODE) == D3DSHADE_FLAT)
			glShadeModel(GL_FLAT);
		else if (VALUE(D3DRS_SHADEMODE) == D3DSHADE_GOURAUD)
			glShadeModel(GL_SMOOTH);
	}
	if (CHANGED(D3DRS_CULLMODE)) {
		// D3D names the winding to cull: make it GL's front face and cull that.
		unsigned int mode = VALUE(D3DRS_CULLMODE);
		if (mode == D3DCULL_CW || mode == D3DCULL_CCW) {
			glFrontFace(mode == D3DCULL_CW ? GL_CW : GL_CCW);
			glCullFace(GL_FRONT);
			glEnable(GL_CULL_FACE);
		} else if (mode == D3DCULL_NONE) {
			glDisable(GL_CULL_FACE);
		}
	}
	float c[4];
	if (CHANGED(D3DRS_FOGCOLOR)) {
		_colorToFloats(VALUE(D3DRS_FOGCOLOR), c);
		c[3] = 1.0f;
		glFogfv(GL_FOG_COLOR, c);
	}
	if (CHANGED(D3DRS_AMBIENT)) {
		_colorToFloats(VALUE(D3DRS_AMBIENT), c);
		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, c);
	}
	if (CHANGED(D3DRS_COLORWRITEENABLE)) {
		unsigned int m = VALUE(D3DRS_COLORWRITEENABLE);
		glColorMask(m & 1, (m >> 1) & 1, (m >> 2) & 1, (m >> 3) & 1);
	}
	if (CHANGED(D3DRS_BLENDFACTOR)) {
		_colorToFloats(VALUE(D3DRS_BLENDFACTOR), c);
		glBlendColor(c[0], c[1], c[2], c[3]);
	}
	if (CHANGED(D3DRS_CLIPPING) || CHANGED(D3DRS_CLIPPLANEENABLE)) {
		unsigned int planes = VALUE(D3DRS_CLIPPING) ? VALUE(D3DRS_CLIPPLANEENABLE) : 0;
		for (int i = 0; i < 6; i++)
			_enableIf(GL_CLIP_PLANE0 + i, (planes >> i) & 1);
	}
	if (CHANGED(D3DRS_FOGVERTEXMODE)) {
		switch (VALUE(D3DRS_FOGVERTEXMODE)) {
		case D3DFOG_EXP:
			glFogi(GL_FOG_MODE, GL_EXP);
			break;
		case D3DFOG_EXP2:
			glFogi(GL_FOG_MODE, GL_EXP2);
			break;
		case D3DFOG_LINEAR:
			glFogi(GL_FOG_MODE, GL_LINEAR);
			break;
		}
	}
	if (CHANGED(D3DRS_FOGENABLE))
		_enableIf(GL_FOG, VALUE(D3DRS_FOGENABLE) != 0);
	if (CHANGED(D3DRS_SRCBLEND) || CHANGED(D3DRS_DESTBLEND))
		glBlendFunc(_blendMode(VALUE(D3DRS_SRCBLEND)), _blendMode(VALUE(D3DRS_DESTBLEND)));
	if (CHANGED(D3DRS_ALPHAREF) || CHANGED(D3DRS_ALPHAFUNC))
		glAlphaFunc(_cmpFunc(VALUE(D3DRS_ALPHAFUNC)), (float)(VALUE(D3DRS_ALPHAREF) & 0xff) / 255.0f);
	if (CHANGED(D3DRS_STENCILFAIL) || CHANGED(D3DRS_STENCILZFAIL) || CHANGED(D3DRS_STENCILPASS))
		glStencilOp(_stencilOp(VALUE(D3DRS_STENCILFAIL)), _stencilOp(VALUE(D3DRS_STENCILZFAIL)),
		            _stencilOp(VALUE(D3DRS_STENCILPASS)));
	if (CHANGED(D3DRS_STENCILFUNC) || CHANGED(D3DRS_STENCILREF) || CHANGED(D3DRS_STENCILMASK))
		glStencilFunc(_cmpFunc(VALUE(D3DRS_STENCILFUNC)), VALUE(D3DRS_STENCILREF), VALUE(D3DRS_STENCILMASK));
#undef CHANGED
#undef VALUE
	for (int i = 0; i < D3D_MAXRENDERSTATES; i++)
		s[i].changed = false;
}

// ---------------------------------------------------------------------------
// Texture stages map onto GL_COMBINE. Each pass (colour, alpha) has an op
// state and three argument states; an op picks a combine mode and the
// variant of the argument-to-source mapping it needs.

struct STextureStatesRemap {
	unsigned int state;	// the stage state
	bool isAlpha;
	GLenum param;	// COMBINE_RGB/ALPHA, or SOURCEn
	GLenum param2;	// RGB_SCALE/ALPHA_SCALE, or OPERANDn
	GLenum defaultValue;	// the mode or the source for anything unmapped
};

static const STextureStatesRemap _sTextureOpRemap[2] = {
	{ D3DTSS_COLOROP, false, GL_COMBINE_RGB, GL_RGB_SCALE, GL_MODULATE },
	{ D3DTSS_ALPHAOP, true, GL_COMBINE_ALPHA, GL_ALPHA_SCALE, GL_MODULATE },
};

// [variant][pass][ARG1, ARG2, ARG0]
static const STextureStatesRemap _sTextureArgRemap[4][2][3] = {
	{
	    { { D3DTSS_COLORARG1, false, GL_SOURCE0_RGB, GL_OPERAND0_RGB, GL_TEXTURE },
	      { D3DTSS_COLORARG2, false, GL_SOURCE1_RGB, GL_OPERAND1_RGB, GL_PREVIOUS },
	      { D3DTSS_COLORARG0, false, GL_SOURCE2_RGB, GL_OPERAND2_RGB, GL_CONSTANT } },
	    { { D3DTSS_ALPHAARG1, true, GL_SOURCE0_ALPHA, GL_OPERAND0_ALPHA, GL_TEXTURE },
	      { D3DTSS_ALPHAARG2, true, GL_SOURCE1_ALPHA, GL_OPERAND1_ALPHA, GL_PREVIOUS },
	      { D3DTSS_ALPHAARG0, true, GL_SOURCE2_ALPHA, GL_OPERAND2_ALPHA, GL_CONSTANT } },
	},
	{
	    { { D3DTSS_COLORARG1, false, GL_SOURCE0_RGB, GL_OPERAND0_RGB, GL_TEXTURE },
	      { D3DTSS_COLORARG2, false, GL_SOURCE2_RGB, GL_OPERAND2_RGB, GL_CONSTANT },
	      { D3DTSS_COLORARG0, false, GL_SOURCE1_RGB, GL_OPERAND1_RGB, GL_PREVIOUS } },
	    { { D3DTSS_ALPHAARG1, true, GL_SOURCE0_ALPHA, GL_OPERAND0_ALPHA, GL_TEXTURE },
	      { D3DTSS_ALPHAARG2, true, GL_SOURCE2_ALPHA, GL_OPERAND2_ALPHA, GL_CONSTANT },
	      { D3DTSS_ALPHAARG0, true, GL_SOURCE1_ALPHA, GL_OPERAND1_ALPHA, GL_PREVIOUS } },
	},
	{
	    { { D3DTSS_COLORARG1, false, GL_SOURCE2_RGB, GL_OPERAND2_RGB, GL_CONSTANT },
	      { D3DTSS_COLORARG2, false, GL_SOURCE0_RGB, GL_OPERAND0_RGB, GL_TEXTURE },
	      { D3DTSS_COLORARG0, false, GL_SOURCE1_RGB, GL_OPERAND1_RGB, GL_PREVIOUS } },
	    { { D3DTSS_ALPHAARG1, true, GL_SOURCE2_ALPHA, GL_OPERAND2_ALPHA, GL_CONSTANT },
	      { D3DTSS_ALPHAARG2, true, GL_SOURCE0_ALPHA, GL_OPERAND0_ALPHA, GL_TEXTURE },
	      { D3DTSS_ALPHAARG0, true, GL_SOURCE1_ALPHA, GL_OPERAND1_ALPHA, GL_PREVIOUS } },
	},
	{
	    { { D3DTSS_COLORARG1, false, GL_SOURCE1_RGB, GL_OPERAND1_RGB, GL_PREVIOUS },
	      { D3DTSS_COLORARG2, false, GL_SOURCE2_RGB, GL_OPERAND2_RGB, GL_CONSTANT },
	      { D3DTSS_COLORARG0, false, GL_SOURCE0_RGB, GL_OPERAND0_RGB, GL_TEXTURE } },
	    { { D3DTSS_ALPHAARG1, true, GL_SOURCE1_ALPHA, GL_OPERAND1_ALPHA, GL_PREVIOUS },
	      { D3DTSS_ALPHAARG2, true, GL_SOURCE2_ALPHA, GL_OPERAND2_ALPHA, GL_CONSTANT },
	      { D3DTSS_ALPHAARG0, true, GL_SOURCE0_ALPHA, GL_OPERAND0_ALPHA, GL_TEXTURE } },
	},
};

// Sets the pass's combine mode for a D3D op; returns the argument variant.
static int _setTextureOp(const STextureStatesRemap *remap, unsigned int op, DeviceEmu *device)
{
	GLenum mode = remap->defaultValue;
	float scale = 1.0f;
	int variant = 0;
	switch (op) {
	case D3DTOP_SELECTARG1:
		mode = GL_REPLACE;
		break;
	case D3DTOP_SELECTARG2:
		mode = GL_REPLACE;
		variant = 2;
		break;
	case D3DTOP_MODULATE:
		mode = GL_MODULATE;
		break;
	case D3DTOP_MODULATE2X:
		mode = GL_MODULATE;
		scale = 2.0f;
		break;
	case D3DTOP_MODULATE4X:
		mode = GL_MODULATE;
		scale = 4.0f;
		break;
	case D3DTOP_ADD:
		mode = GL_ADD;
		break;
	case D3DTOP_ADDSIGNED:
		mode = GL_ADD_SIGNED;
		break;
	case D3DTOP_ADDSIGNED2X:
		mode = GL_ADD_SIGNED;
		scale = 2.0f;
		break;
	case D3DTOP_SUBTRACT:
		mode = GL_SUBTRACT;
		break;
	case D3DTOP_ADDSMOOTH:	// arg0 (made 1-arg1 by the caller) * arg2 + arg1
		mode = GL_MODULATE_ADD_ATI;
		variant = 3;
		break;
	case D3DTOP_DOTPRODUCT3:
		if (device->contextCaps()->textureEnvDot3IsSupported() && !remap->isAlpha)
			mode = GL_DOT3_RGBA;
		break;
	case D3DTOP_MULTIPLYADD:	// arg1 * arg2 + arg0
		mode = GL_MODULATE_ADD_ATI;
		variant = 1;
		break;
	case D3DTOP_LERP:
		mode = GL_INTERPOLATE;
		break;
	default:
		break;
	}
	glTexEnvi(GL_TEXTURE_ENV, remap->param, mode);
	glTexEnvf(GL_TEXTURE_ENV, remap->param2, scale);
	return variant;
}

static void _setTextureArg(const STextureStatesRemap *remap, unsigned int arg, DeviceEmu *)
{
	GLenum source;
	switch (arg & D3DTA_SELECTMASK) {
	case D3DTA_DIFFUSE:
		source = GL_PRIMARY_COLOR;
		break;
	case D3DTA_CURRENT:
		source = GL_PREVIOUS;
		break;
	case D3DTA_TEXTURE:
		source = GL_TEXTURE;
		break;
	case D3DTA_TFACTOR:
	case D3DTA_CONSTANT:
		source = GL_CONSTANT;
		break;
	default:
		source = remap->defaultValue;
		break;
	}
	GLenum operand;
	if (remap->isAlpha)
		operand = (arg & D3DTA_COMPLEMENT) ? GL_ONE_MINUS_SRC_ALPHA : GL_SRC_ALPHA;
	else if (arg & D3DTA_COMPLEMENT)
		operand = GL_ONE_MINUS_SRC_COLOR;
	else
		operand = (arg & D3DTA_ALPHAREPLICATE) ? GL_SRC_ALPHA : GL_SRC_COLOR;
	glTexEnvi(GL_TEXTURE_ENV, remap->param, source);
	glTexEnvi(GL_TEXTURE_ENV, remap->param2, operand);
}

bool DeviceStatesRemap::applyTextureStates(STextureStates *ts, DeviceEmu *device)
{
	SStateValue *s = ts->s;
	bool enabled = true;
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	for (int pass = 0; pass < 2; pass++) {
		const STextureStatesRemap *opRemap = &_sTextureOpRemap[pass];
		SStateValue &op = s[opRemap->state];
		if (!op.changed)
			continue;
		bool isAlpha = opRemap->isAlpha;
		// Ops GL_COMBINE lacks are rewritten into ones it has.
		switch (op.value) {
		case D3DTOP_DISABLE: {
			op.value = D3DTOP_SELECTARG1;
			SStateValue &arg1 = s[isAlpha ? D3DTSS_ALPHAARG1 : D3DTSS_COLORARG1];
			arg1.changed = true;
			arg1.value = D3DTA_CURRENT;
			enabled = false;
			break;
		}
		case D3DTOP_BLENDCURRENTALPHA:
		case D3DTOP_BLENDDIFFUSEALPHA:
		case D3DTOP_BLENDTEXTUREALPHA:
			if (!isAlpha) {
				unsigned int arg0 = op.value == D3DTOP_BLENDCURRENTALPHA   ? (D3DTA_CURRENT | D3DTA_ALPHAREPLICATE)
				                    : op.value == D3DTOP_BLENDDIFFUSEALPHA ? (D3DTA_DIFFUSE | D3DTA_ALPHAREPLICATE)
				                                                           : (D3DTA_TEXTURE | D3DTA_ALPHAREPLICATE);
				op.value = D3DTOP_LERP;
				s[D3DTSS_COLORARG0].changed = true;
				s[D3DTSS_COLORARG0].value = arg0;
			}
			break;
		case D3DTOP_ADDSMOOTH: {
			unsigned int arg1 = s[isAlpha ? D3DTSS_ALPHAARG1 : D3DTSS_COLORARG1].value;
			SStateValue &arg0 = s[isAlpha ? D3DTSS_ALPHAARG0 : D3DTSS_COLORARG0];
			arg0.changed = true;
			arg0.value = arg1 ^ D3DTA_COMPLEMENT;
			break;
		}
		default:
			break;
		}
		int variant = _setTextureOp(opRemap, op.value, device);
		for (int i = 0; i < 3; i++) {
			const STextureStatesRemap *argRemap = &_sTextureArgRemap[variant][pass][i];
			if (s[argRemap->state].changed)
				_setTextureArg(argRemap, s[argRemap->state].value, device);
		}
	}
	if (s[D3DTSS_CONSTANT].changed) {
		float c[4];
		_colorToFloats(s[D3DTSS_CONSTANT].value, c);
		glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, c);
	}
	for (int i = 0; i < D3D_MAXTEXTURESTAGESTATES; i++)
		s[i].changed = false;
	return enabled;
}

// ---------------------------------------------------------------------------

void DeviceStatesRemap::applySamplerStates(GLenum target, SSamplerStates *ss, DeviceEmu *device)
{
	SStateValue *s = ss->s;
	if (s[D3DSAMP_ADDRESSU].changed)
		glTexParameteri(target, GL_TEXTURE_WRAP_S, _glTextureWrapMode(s[D3DSAMP_ADDRESSU].value));
	if (s[D3DSAMP_ADDRESSV].changed)
		glTexParameteri(target, GL_TEXTURE_WRAP_T, _glTextureWrapMode(s[D3DSAMP_ADDRESSV].value));
	if (s[D3DSAMP_ADDRESSW].changed)
		glTexParameteri(target, GL_TEXTURE_WRAP_R, _glTextureWrapMode(s[D3DSAMP_ADDRESSW].value));
	if (s[D3DSAMP_BORDERCOLOR].changed) {
		float c[4];
		_colorToFloats(s[D3DSAMP_BORDERCOLOR].value, c);
		glTexParameterfv(target, GL_TEXTURE_BORDER_COLOR, c);
	}
	if (s[D3DSAMP_MAGFILTER].changed)
		glTexParameteri(target, GL_TEXTURE_MAG_FILTER,
		                s[D3DSAMP_MAGFILTER].value == D3DTEXF_POINT ? GL_NEAREST : GL_LINEAR);
	if (s[D3DSAMP_MINFILTER].changed || s[D3DSAMP_MIPFILTER].changed) {
		bool linear = s[D3DSAMP_MINFILTER].value != D3DTEXF_POINT;
		GLint filter = linear ? GL_LINEAR : GL_NEAREST;
		bool mipmaps = false;
		if (s[D3DSAMP_MIPFILTER].value == D3DTEXF_POINT) {
			filter = linear ? GL_LINEAR_MIPMAP_NEAREST : GL_NEAREST_MIPMAP_NEAREST;
			mipmaps = true;
		} else if (s[D3DSAMP_MIPFILTER].value == D3DTEXF_LINEAR) {
			filter = linear ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_LINEAR;
			mipmaps = true;
		}
		// The levels below the first are generated by GL.
		glTexParameteri(target, GL_GENERATE_MIPMAP, mipmaps);
		glTexParameteri(target, GL_TEXTURE_MIN_FILTER, filter);
	}
	if (s[D3DSAMP_MIPMAPLODBIAS].changed && device->contextCaps()->textureLodBiasIsSupported())
		glTexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, _asFloat(s[D3DSAMP_MIPMAPLODBIAS].value));
	if (s[D3DSAMP_MAXMIPLEVEL].changed)
		glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, s[D3DSAMP_MAXMIPLEVEL].value);
}
