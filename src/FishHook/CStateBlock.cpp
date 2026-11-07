// CStateBlock: device states recorded into a StateBlockEmu, from a table or
// from arrays of strings ("RenderState", "D3DRS_ZENABLE", "D3DZB_TRUE"...).

#include "FHContext.h"
#include "LValue.h"
#include <string.h>

LF_IMPLEMENT_CLASS(CStateBlock, LF_CLASSOF(CContextObject))

enum { kRenderStateItem = 1, kTextureStageStateItem = 2, kSamplerStateItem = 3 };

#define RS(state, value) { kRenderStateItem, state, value, 0 }
#define TSS(stage, state, value) { kTextureStageStateItem, stage, state, value }
#define SAMP(stage, state, value) { kSamplerStateItem, stage, state, value }

const SStateItemDescr CStateBlock::_sDefaultStateDescr[] = {
	RS(D3DRS_LIGHTING, 1),
	RS(D3DRS_ZENABLE, 1),
	RS(D3DRS_FILLMODE, D3DFILL_SOLID),
	RS(D3DRS_ZWRITEENABLE, 1),
	RS(D3DRS_ALPHATESTENABLE, 0),
	RS(D3DRS_DITHERENABLE, 1),
	RS(D3DRS_ALPHABLENDENABLE, 0),
	RS(D3DRS_CULLMODE, D3DCULL_NONE),
	RS(D3DRS_FOGENABLE, 0),
	RS(D3DRS_FOGSTART, 0x3f800000),	// 1.0
	RS(D3DRS_FOGEND, 0),
	RS(D3DRS_FOGDENSITY, 0x3f800000),	// 1.0
	RS(D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR),
	RS(D3DRS_COLORWRITEENABLE, 0xf),
	TSS(0, D3DTSS_TEXCOORDINDEX, 0),
	TSS(1, D3DTSS_TEXCOORDINDEX, 1),
	TSS(2, D3DTSS_TEXCOORDINDEX, 2),
	TSS(3, D3DTSS_TEXCOORDINDEX, 3),
	SAMP(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP),
	SAMP(1, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP),
	SAMP(2, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP),
	SAMP(3, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP),
	SAMP(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP),
	SAMP(1, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP),
	SAMP(2, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP),
	SAMP(3, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP),
	SAMP(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR),
	SAMP(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR),
	SAMP(2, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR),
	SAMP(3, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR),
	SAMP(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR),
	SAMP(1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR),
	SAMP(2, D3DSAMP_MINFILTER, D3DTEXF_LINEAR),
	SAMP(3, D3DSAMP_MINFILTER, D3DTEXF_LINEAR),
	SAMP(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR),
	SAMP(1, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR),
	SAMP(2, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR),
	SAMP(3, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR),
};
const long CStateBlock::_sDefaultStateItemsCount = sizeof(_sDefaultStateDescr) / sizeof(_sDefaultStateDescr[0]);

// Name maps for setStaticStatesArray.
static CSharableResource sStdMapsLocker;
static LMap *sItemTypesMap = 0;
static LMap *sRenderStatesMap = 0;
static LMap *sSamplerStatesMap = 0;
static LMap *sTxtStageStatesMap = 0;
static LMap *sConstantsMap = 0;

class LStateBlocksCleaner : public LCleanupTask {
public:
	virtual void performCleanup()
	{
		LMap **maps[] = { &sItemTypesMap, &sRenderStatesMap, &sSamplerStatesMap, &sTxtStageStatesMap,
			          &sConstantsMap };
		for (unsigned int i = 0; i < sizeof(maps) / sizeof(maps[0]); i++) {
			LRelease(*maps[i]);
			*maps[i] = 0;
		}
	}
};
static LStateBlocksCleaner _state_blocks_cleaner;

CStateBlock::CStateBlock() : CContextObject(0), _itemsBlock(0), _items(0), _itemsCount(0), _block(0)
{
	_managesSelf = true;
}

CStateBlock *CStateBlock::initWithHolder(CHolder *holder)
{
	CContextObject::initWithHolder(holder);
	return this;
}

CStateBlock *CStateBlock::initWithStateDescriptors(const SStateItemDescr *items, long count, CHolder *holder)
{
	CContextObject::initWithHolder(holder);
	if (count < 1)
		LF_RAISE(WrongConditionException());
	_itemsBlock = new StPointerBlock(count * sizeof(SStateItemDescr), false);
	_items = (SStateItemDescr *)_itemsBlock->bytes();
	_itemsCount = count;
	memcpy(_items, items, _itemsBlock->size());
	return this;
}

CStateBlock *CStateBlock::initWithArray(LArray *array, CHolder *holder)
{
	CContextObject::initWithHolder(holder);
	setStaticStatesArray(array);
	return this;
}

void CStateBlock::dealloc()
{
	delete _itemsBlock;
	_itemsBlock = 0;
	CContextObject::dealloc();
}

void CStateBlock::instantiateSelf()
{
	if (_block == 0 && _itemsCount != 0) {
		beginStateRecording();
		endStateRecording();
	}
}

void CStateBlock::releaseSelf()
{
	FHSafeRelease(_block);
}

void CStateBlock::applySelf()
{
	if (_block)
		_block->Apply();
}

bool CStateBlock::beginStateRecording()
{
	if (_context == 0)
		LF_RAISE(NilPtrException());
	FHSafeRelease(_block);
	DeviceEmu *device = _context->device();
	HRESULT hr = device->BeginStateBlock();
	if (_itemsCount > 0)
		appendStates(_items, _itemsCount, device);
	return hr >= 0;
}

void CStateBlock::endStateRecording()
{
	_context->device()->EndStateBlock(&_block);
}

void CStateBlock::appendStates(const SStateItemDescr *items, long count, DeviceEmu *device)
{
	for (long i = 0; i < count; i++) {
		const SStateItemDescr &item = items[i];
		switch (item.type) {
		case kTextureStageStateItem:
			device->SetTextureStageState(item.a, (D3DTEXTURESTAGESTATETYPE)item.b, item.c);
			break;
		case kSamplerStateItem:
			device->SetSamplerState(item.a, (D3DSAMPLERSTATETYPE)item.b, item.c);
			break;
		case kRenderStateItem:
			device->SetRenderState((D3DRENDERSTATETYPE)item.a, item.b);
			break;
		}
	}
}

CStateBlock *CStateBlock::defaultStateWithContext(CContext *context)
{
	CStateBlock *block = LAutorelease(
	    CStateBlock::_alloc()->initWithStateDescriptors(_sDefaultStateDescr, _sDefaultStateItemsCount, 0));
	block->instantiateInContext(context);
	return block;
}

// ---------------------------------------------------------------------------
// Parsing.

bool CStateBlock::renderStateRequiresFloatParam(int state)
{
	switch (state) {
	case D3DRS_FOGSTART:
	case D3DRS_FOGEND:
	case D3DRS_FOGDENSITY:
	case D3DRS_POINTSIZE:
	case D3DRS_POINTSIZE_MIN:
	case D3DRS_POINTSCALE_A:
	case D3DRS_POINTSCALE_B:
	case D3DRS_POINTSCALE_C:
	case D3DRS_POINTSIZE_MAX:
	case D3DRS_TWEENFACTOR:
	case D3DRS_DEPTHBIAS:
		return true;
	default:
		return false;
	}
}

// The bump-mapping matrix and luminance states.
bool CStateBlock::txtStageStateRequiresFloatParam(unsigned int state)
{
	return state <= 0x17 && ((1u << state) & 0xc00780u) != 0;
}

static void _insert(LMap *map, int value, const char *name)
{
	InsertIntValueForKey(map, value, name);
}

static LMap *_newMap()
{
	return LRetain((LMap *)TLMap<LValue *, LString *>::map());
}

void CStateBlock::createMapsIfNeeded()
{
	StSharableLocker lock(&sStdMapsLocker);
	if (sItemTypesMap == 0) {
		sItemTypesMap = _newMap();
		_insert(sItemTypesMap, kRenderStateItem, "RenderState");
		_insert(sItemTypesMap, kTextureStageStateItem, "TextureStageState");
		_insert(sItemTypesMap, kSamplerStateItem, "SamplerState");
	}
	if (sRenderStatesMap == 0) {
		static const struct {
			int value;
			const char *name;
		} kRenderStates[] = {
			{ 7, "D3DRS_ZENABLE" },
			{ 8, "D3DRS_FILLMODE" },
			{ 9, "D3DRS_SHADEMODE" },
			{ 0xe, "D3DRS_ZWRITEENABLE" },
			{ 0xf, "D3DRS_ALPHATESTENABLE" },
			{ 0x10, "D3DRS_LASTPIXEL" },
			{ 0x13, "D3DRS_SRCBLEND" },
			{ 0x14, "D3DRS_DESTBLEND" },
			{ 0x16, "D3DRS_CULLMODE" },
			{ 0x17, "D3DRS_ZFUNC" },
			{ 0x18, "D3DRS_ALPHAREF" },
			{ 0x19, "D3DRS_ALPHAFUNC" },
			{ 0x1a, "D3DRS_DITHERENABLE" },
			{ 0x1b, "D3DRS_ALPHABLENDENABLE" },
			{ 0x1c, "D3DRS_FOGENABLE" },
			{ 0x1d, "D3DRS_SPECULARENABLE" },
			{ 0x22, "D3DRS_FOGCOLOR" },
			{ 0x23, "D3DRS_FOGTABLEMODE" },
			{ 0x24, "D3DRS_FOGSTART" },
			{ 0x25, "D3DRS_FOGEND" },
			{ 0x26, "D3DRS_FOGDENSITY" },
			{ 0x30, "D3DRS_RANGEFOGENABLE" },
			{ 0x34, "D3DRS_STENCILENABLE" },
			{ 0x35, "D3DRS_STENCILFAIL" },
			{ 0x36, "D3DRS_STENCILZFAIL" },
			{ 0x37, "D3DRS_STENCILPASS" },
			{ 0x38, "D3DRS_STENCILFUNC" },
			{ 0x39, "D3DRS_STENCILREF" },
			{ 0x3a, "D3DRS_STENCILMASK" },
			{ 0x3b, "D3DRS_STENCILWRITEMASK" },
			{ 0x3c, "D3DRS_TEXTUREFACTOR" },
			{ 0x80, "D3DRS_WRAP0" },
			{ 0x81, "D3DRS_WRAP1" },
			{ 0x82, "D3DRS_WRAP2" },
			{ 0x83, "D3DRS_WRAP3" },
			{ 0x84, "D3DRS_WRAP4" },
			{ 0x85, "D3DRS_WRAP5" },
			{ 0x86, "D3DRS_WRAP6" },
			{ 0x87, "D3DRS_WRAP7" },
			{ 0x88, "D3DRS_CLIPPING" },
			{ 0x89, "D3DRS_LIGHTING" },
			{ 0x8b, "D3DRS_AMBIENT" },
			{ 0x8c, "D3DRS_FOGVERTEXMODE" },
			{ 0x8d, "D3DRS_COLORVERTEX" },
			{ 0x8e, "D3DRS_LOCALVIEWER" },
			{ 0x8f, "D3DRS_NORMALIZENORMALS" },
			{ 0x91, "D3DRS_DIFFUSEMATERIALSOURCE" },
			{ 0x92, "D3DRS_SPECULARMATERIALSOURCE" },
			{ 0x93, "D3DRS_AMBIENTMATERIALSOURCE" },
			{ 0x94, "D3DRS_EMISSIVEMATERIALSOURCE" },
			{ 0x97, "D3DRS_VERTEXBLEND" },
			{ 0x98, "D3DRS_CLIPPLANEENABLE" },
			{ 0x9a, "D3DRS_POINTSIZE" },
			{ 0x9b, "D3DRS_POINTSIZE_MIN" },
			{ 0x9c, "D3DRS_POINTSPRITEENABLE" },
			{ 0x9d, "D3DRS_POINTSCALEENABLE" },
			{ 0x9e, "D3DRS_POINTSCALE_A" },
			{ 0x9f, "D3DRS_POINTSCALE_B" },
			{ 0xa0, "D3DRS_POINTSCALE_C" },
			{ 0xa1, "D3DRS_MULTISAMPLEANTIALIAS" },
			{ 0xa2, "D3DRS_MULTISAMPLEMASK" },
			{ 0xa3, "D3DRS_PATCHEDGESTYLE" },
			{ 0xa5, "D3DRS_DEBUGMONITORTOKEN" },
			{ 0xa6, "D3DRS_POINTSIZE_MAX" },
			{ 0xa7, "D3DRS_INDEXEDVERTEXBLENDENABLE" },
			{ 0xa8, "D3DRS_COLORWRITEENABLE" },
			{ 0xaa, "D3DRS_TWEENFACTOR" },
			{ 0xab, "D3DRS_BLENDOP" },
			{ 0xac, "D3DRS_POSITIONDEGREE" },
			{ 0xad, "D3DRS_NORMALDEGREE" },
			{ 0xae, "D3DRS_SCISSORTESTENABLE" },
			{ 0xaf, "D3DRS_SLOPESCALEDEPTHBIAS" },
			{ 0xb0, "D3DRS_ANTIALIASEDLINEENABLE" },
			{ 0xb2, "D3DRS_MINTESSELLATIONLEVEL" },
			{ 0xb3, "D3DRS_MAXTESSELLATIONLEVEL" },
			{ 0xb4, "D3DRS_ADAPTIVETESS_X" },
			{ 0xb5, "D3DRS_ADAPTIVETESS_Y" },
			{ 0xb6, "D3DRS_ADAPTIVETESS_Z" },
			{ 0xb7, "D3DRS_ADAPTIVETESS_W" },
			{ 0xb8, "D3DRS_ENABLEADAPTIVETESSELLATION" },
			{ 0xb9, "D3DRS_TWOSIDEDSTENCILMODE" },
			{ 0xba, "D3DRS_CCW_STENCILFAIL" },
			{ 0xbb, "D3DRS_CCW_STENCILZFAIL" },
			{ 0xbc, "D3DRS_CCW_STENCILPASS" },
			{ 0xbd, "D3DRS_CCW_STENCILFUNC" },
			{ 0xbe, "D3DRS_COLORWRITEENABLE1" },
			{ 0xbf, "D3DRS_COLORWRITEENABLE2" },
			{ 0xc0, "D3DRS_COLORWRITEENABLE3" },
			{ 0xc1, "D3DRS_BLENDFACTOR" },
			{ 0xc2, "D3DRS_SRGBWRITEENABLE" },
			{ 0xc3, "D3DRS_DEPTHBIAS" },
			{ 0xc6, "D3DRS_WRAP8" },
			{ 0xc7, "D3DRS_WRAP9" },
			{ 0xc8, "D3DRS_WRAP10" },
			{ 0xc9, "D3DRS_WRAP11" },
			{ 0xca, "D3DRS_WRAP12" },
			{ 0xcb, "D3DRS_WRAP13" },
			{ 0xcc, "D3DRS_WRAP14" },
			{ 0xcd, "D3DRS_WRAP15" },
			{ 0xce, "D3DRS_SEPARATEALPHABLENDENABLE" },
			{ 0xcf, "D3DRS_SRCBLENDALPHA" },
			{ 0xd0, "D3DRS_DESTBLENDALPHA" },
			{ 0xd1, "D3DRS_BLENDOPALPHA" },
		};
		sRenderStatesMap = _newMap();
		for (unsigned int i = 0; i < sizeof(kRenderStates) / sizeof(kRenderStates[0]); i++)
			_insert(sRenderStatesMap, kRenderStates[i].value, kRenderStates[i].name);
	}
	if (sSamplerStatesMap == 0) {
		static const char *const kSamplerStates[] = {
			"D3DSAMP_ADDRESSU",	 "D3DSAMP_ADDRESSV",  "D3DSAMP_ADDRESSW",	   "D3DSAMP_BORDERCOLOR",
			"D3DSAMP_MAGFILTER",	 "D3DSAMP_MINFILTER", "D3DSAMP_MIPFILTER",	   "D3DSAMP_MIPMAPLODBIAS",
			"D3DSAMP_MAXMIPLEVEL",	 "D3DSAMP_MAXANISOTROPY", "D3DSAMP_SRGBTEXTURE", "D3DSAMP_ELEMENTINDEX",
			"D3DSAMP_DMAPOFFSET",
		};
		sSamplerStatesMap = _newMap();
		for (unsigned int i = 0; i < sizeof(kSamplerStates) / sizeof(kSamplerStates[0]); i++)
			_insert(sSamplerStatesMap, i + 1, kSamplerStates[i]);
	}
	if (sTxtStageStatesMap == 0) {
		static const struct {
			int value;
			const char *name;
		} kStageStates[] = {
			{ 1, "D3DTSS_COLOROP" },
			{ 2, "D3DTSS_COLORARG1" },
			{ 3, "D3DTSS_COLORARG2" },
			{ 4, "D3DTSS_ALPHAOP" },
			{ 5, "D3DTSS_ALPHAARG1" },
			{ 6, "D3DTSS_ALPHAARG2" },
			{ 7, "D3DTSS_BUMPENVMAT00" },
			{ 8, "D3DTSS_BUMPENVMAT01" },
			{ 9, "D3DTSS_BUMPENVMAT10" },
			{ 10, "D3DTSS_BUMPENVMAT11" },
			{ 0xb, "D3DTSS_TEXCOORDINDEX" },
			{ 0x16, "D3DTSS_BUMPENVLSCALE" },
			{ 0x17, "D3DTSS_BUMPENVLOFFSET" },
			{ 0x18, "D3DTSS_TEXTURETRANSFORMFLAGS" },
			{ 0x1a, "D3DTSS_COLORARG0" },
			{ 0x1b, "D3DTSS_ALPHAARG0" },
			{ 0x1c, "D3DTSS_RESULTARG" },
			{ 0x20, "D3DTSS_CONSTANT" },
		};
		sTxtStageStatesMap = _newMap();
		for (unsigned int i = 0; i < sizeof(kStageStates) / sizeof(kStageStates[0]); i++)
			_insert(sTxtStageStatesMap, kStageStates[i].value, kStageStates[i].name);
	}
	if (sConstantsMap == 0) {
		static const struct {
			int value;
			const char *name;
		} kConstants[] = {
			{ 1, "D3DTOP_DISABLE" },
			{ 2, "D3DTOP_SELECTARG1" },
			{ 3, "D3DTOP_SELECTARG2" },
			{ 4, "D3DTOP_MODULATE" },
			{ 5, "D3DTOP_MODULATE2X" },
			{ 6, "D3DTOP_MODULATE4X" },
			{ 7, "D3DTOP_ADD" },
			{ 8, "D3DTOP_ADDSIGNED" },
			{ 9, "D3DTOP_ADDSIGNED2X" },
			{ 10, "D3DTOP_SUBTRACT" },
			{ 0xb, "D3DTOP_ADDSMOOTH" },
			{ 0xc, "D3DTOP_BLENDDIFFUSEALPHA" },
			{ 0xd, "D3DTOP_BLENDTEXTUREALPHA" },
			{ 0xe, "D3DTOP_BLENDFACTORALPHA" },
			{ 0xf, "D3DTOP_BLENDTEXTUREALPHAPM" },
			{ 0x10, "D3DTOP_BLENDCURRENTALPHA" },
			{ 0x11, "D3DTOP_PREMODULATE" },
			{ 0x12, "D3DTOP_MODULATEALPHA_ADDCOLOR" },
			{ 0x13, "D3DTOP_MODULATECOLOR_ADDALPHA" },
			{ 0x14, "D3DTOP_MODULATEINVALPHA_ADDCOLOR" },
			{ 0x15, "D3DTOP_MODULATEINVCOLOR_ADDALPHA" },
			{ 0x16, "D3DTOP_BUMPENVMAP" },
			{ 0x17, "D3DTOP_BUMPENVMAPLUMINANCE" },
			{ 0x18, "D3DTOP_DOTPRODUCT3" },
			{ 0x19, "D3DTOP_MULTIPLYADD" },
			{ 0x1a, "D3DTOP_LERP" },
			{ 0xf, "D3DTA_SELECTMASK" },
			{ 0, "D3DTA_DIFFUSE" },
			{ 1, "D3DTA_CURRENT" },
			{ 2, "D3DTA_TEXTURE" },
			{ 3, "D3DTA_TFACTOR" },
			{ 4, "D3DTA_SPECULAR" },
			{ 5, "D3DTA_TEMP" },
			{ 6, "D3DTA_CONSTANT" },
			{ 0x10, "D3DTA_COMPLEMENT" },
			{ 0x20, "D3DTA_ALPHAREPLICATE" },
			{ 0, "D3DTTFF_DISABLE" },
			{ 1, "D3DTTFF_COUNT1" },
			{ 2, "D3DTTFF_COUNT2" },
			{ 3, "D3DTTFF_COUNT3" },
			{ 4, "D3DTTFF_COUNT4" },
			{ 0x100, "D3DTTFF_PROJECTED" },
			{ 1, "D3DTADDRESS_WRAP" },
			{ 2, "D3DTADDRESS_MIRROR" },
			{ 3, "D3DTADDRESS_CLAMP" },
			{ 4, "D3DTADDRESS_BORDER" },
			{ 5, "D3DTADDRESS_MIRRORONCE" },
			{ 0, "D3DTEXF_NONE" },
			{ 1, "D3DTEXF_POINT" },
			{ 2, "D3DTEXF_LINEAR" },
			{ 3, "D3DTEXF_ANISOTROPIC" },
			{ 6, "D3DTEXF_PYRAMIDALQUAD" },
			{ 7, "D3DTEXF_GAUSSIANQUAD" },
			{ 0, "D3DTSS_TCI_PASSTHRU" },
			{ 0x10000, "D3DTSS_TCI_CAMERASPACENORMAL" },
			{ 0x20000, "D3DTSS_TCI_CAMERASPACEPOSITION" },
			{ 0x30000, "D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR" },
			{ 0x40000, "D3DTSS_TCI_SPHEREMAP" },
			{ 0, "D3DZB_FALSE" },
			{ 1, "D3DZB_TRUE" },
			{ 2, "D3DZB_USEW" },
			{ 1, "D3DFILL_POINT" },
			{ 2, "D3DFILL_WIREFRAME" },
			{ 3, "D3DFILL_SOLID" },
			{ 1, "D3DSHADE_FLAT" },
			{ 2, "D3DSHADE_GOURAUD" },
			{ 3, "D3DSHADE_PHONG" },
			{ 1, "D3DBLEND_ZERO" },
			{ 2, "D3DBLEND_ONE" },
			{ 3, "D3DBLEND_SRCCOLOR" },
			{ 4, "D3DBLEND_INVSRCCOLOR" },
			{ 5, "D3DBLEND_SRCALPHA" },
			{ 6, "D3DBLEND_INVSRCALPHA" },
			{ 7, "D3DBLEND_DESTALPHA" },
			{ 8, "D3DBLEND_INVDESTALPHA" },
			{ 9, "D3DBLEND_DESTCOLOR" },
			{ 10, "D3DBLEND_INVDESTCOLOR" },
			{ 0xb, "D3DBLEND_SRCALPHASAT" },
			{ 0xc, "D3DBLEND_BOTHSRCALPHA" },
			{ 0xd, "D3DBLEND_BOTHINVSRCALPHA" },
			{ 0xe, "D3DBLEND_BLENDFACTOR" },
			{ 0xf, "D3DBLEND_INVBLENDFACTOR" },
			{ 1, "D3DCULL_NONE" },
			{ 2, "D3DCULL_CW" },
			{ 3, "D3DCULL_CCW" },
			{ 1, "D3DCMP_NEVER" },
			{ 2, "D3DCMP_LESS" },
			{ 3, "D3DCMP_EQUAL" },
			{ 4, "D3DCMP_LESSEQUAL" },
			{ 5, "D3DCMP_GREATER" },
			{ 6, "D3DCMP_NOTEQUAL" },
			{ 7, "D3DCMP_GREATEREQUAL" },
			{ 8, "D3DCMP_ALWAYS" },
			{ 1, "D3DSTENCILOP_KEEP" },
			{ 2, "D3DSTENCILOP_ZERO" },
			{ 3, "D3DSTENCILOP_REPLACE" },
			{ 4, "D3DSTENCILOP_INCRSAT" },
			{ 5, "D3DSTENCILOP_DECRSAT" },
			{ 6, "D3DSTENCILOP_INVERT" },
			{ 7, "D3DSTENCILOP_INCR" },
			{ 8, "D3DSTENCILOP_DECR" },
			{ 1, "D3DDEGREE_LINEAR" },
			{ 2, "D3DDEGREE_QUADRATIC" },
			{ 3, "D3DDEGREE_CUBIC" },
			{ 5, "D3DDEGREE_QUINTIC" },
		};
		sConstantsMap = _newMap();
		for (unsigned int i = 0; i < sizeof(kConstants) / sizeof(kConstants[0]); i++)
			_insert(sConstantsMap, kConstants[i].value, kConstants[i].name);
	}
}

bool CStateBlock::dxConstantByString(LString *name, long &value)
{
	if (name == 0)
		return false;
	if (sConstantsMap == 0)
		createMapsIfNeeded();
	LValue *v = lobj_cast<LValue>(sConstantsMap->objectForKey(name));
	if (v == 0)
		return false;
	value = v->intValue();
	return true;
}

static unsigned int _floatBits(float f)
{
	unsigned int bits;
	memcpy(&bits, &f, 4);
	return bits;
}

// A value string: a float for float states, else a constant's name or a number.
static unsigned int _stateValue(LString *s, bool isFloat)
{
	if (isFloat)
		return _floatBits(s->floatValue());
	long value;
	if (!CStateBlock::dxConstantByString(s, value))
		value = s->intValue();
	return (unsigned int)value;
}

void CStateBlock::setStaticStatesArray(LArray *array)
{
	int count = array->count();
	TArray<SStateItemDescr> items;
	createMapsIfNeeded();
	StSharableLocker lock(&sStdMapsLocker);
	for (int i = 0; i < count; i++) {
		LArray *item = lobj_cast<LArray>(array->objectAtIndex(i));
		if (item == 0)
			LF_RAISE(WrongConditionException());
		LValue *type = (LValue *)sItemTypesMap->objectForKey((LString *)item->objectAtIndex(0));
		if (type == 0)
			continue;
		SStateItemDescr d;
		switch (type->intValue()) {
		case kTextureStageStateItem: {
			if (item->count() != 4)
				continue;
			LValue *state = (LValue *)sTxtStageStatesMap->objectForKey((LString *)item->objectAtIndex(2));
			if (state == 0)
				continue;
			d.type = kTextureStageStateItem;
			d.a = ((LString *)item->objectAtIndex(1))->intValue();
			d.b = state->intValue();
			d.c = _stateValue((LString *)item->objectAtIndex(3), txtStageStateRequiresFloatParam(d.b));
			break;
		}
		case kSamplerStateItem: {
			if (item->count() != 4)
				continue;
			LValue *state = (LValue *)sSamplerStatesMap->objectForKey((LString *)item->objectAtIndex(2));
			if (state == 0)
				continue;
			d.type = kSamplerStateItem;
			d.a = ((LString *)item->objectAtIndex(1))->intValue();
			d.b = state->intValue();
			d.c = _stateValue((LString *)item->objectAtIndex(3), samplerStateRequiresFloatParam(d.b));
			break;
		}
		case kRenderStateItem: {
			if (item->count() != 3)
				continue;
			LValue *state = (LValue *)sRenderStatesMap->objectForKey((LString *)item->objectAtIndex(1));
			if (state == 0)
				continue;
			d.type = kRenderStateItem;
			d.a = state->intValue();
			d.b = _stateValue((LString *)item->objectAtIndex(2), renderStateRequiresFloatParam(d.a));
			// Written 0xRRGGBBAA; the device takes 0xAARRGGBB.
			if (d.a == D3DRS_TEXTUREFACTOR)
				d.b = d.b << 24 | (d.b >> 8 & 0xff) | (d.b >> 8 & 0xff00) | (d.b >> 24) << 16;
			d.c = 0;
			break;
		}
		default:
			continue;
		}
		items.addObject(d);
	}
	unsigned int added = items.count();
	if (_itemsBlock == 0)
		_itemsBlock = new StPointerBlock(added * sizeof(SStateItemDescr), false);
	else
		_itemsBlock->setSize((_itemsCount + added) * sizeof(SStateItemDescr));
	_items = (SStateItemDescr *)_itemsBlock->bytes();
	memcpy(_items + _itemsCount, items.items(), added * sizeof(SStateItemDescr));
	_itemsCount += added;
}
