// GLBugFixes and the D3DX-like helper functions.

#include "FHTextures.h"

LF_IMPLEMENT_CLASS(GLBugFixes, LF_CLASSOF(CContextObject))

// ---------------------------------------------------------------------------
// Vertex declarations

unsigned int FHGetSizeForDeclType(int type)
{
	static const unsigned int sizes[18] = { 4, 8, 12, 16, 4, 4, 4, 8, 4, 4, 8, 4, 8, 4, 4, 4, 8, 0 };
	return (unsigned int)type < 18 ? sizes[type] : 0;
}

void FHGetGLParamsFromVertexDeclType(int type, GLenum *glType, GLint *count)
{
	static const GLenum types[18] = { GL_FLOAT, GL_FLOAT, GL_FLOAT, GL_FLOAT, GL_UNSIGNED_BYTE, GL_UNSIGNED_BYTE,
	                                  GL_SHORT, GL_SHORT };
	static const GLint counts[18] = { 1, 2, 3, 4, 4, 4, 2, 4 };
	*glType = (unsigned int)type < 18 ? types[type] : 0;
	*count = (unsigned int)type < 18 ? counts[type] : 0;
}

unsigned int FHGetDeclVertexSize(const D3DVERTEXELEMENT9 *decl, unsigned int stream)
{
	unsigned int size = 0;
	for (int i = 0; i < kMaxDeclElements && decl->Type != D3DDECLTYPE_UNUSED; i++, decl++) {
		if (decl->Stream == stream) {
			unsigned int end = decl->Offset + FHGetSizeForDeclType(decl->Type);
			if (end > size)
				size = end;
		}
	}
	return size;
}

HRESULT FHComputeBoundingBox(const Vector3 *first, unsigned int count, unsigned int stride, Vector3 *min,
                             Vector3 *max)
{
	*min = *first;
	*max = *first;
	const char *p = (const char *)first;
	for (unsigned int i = 1; i < count; i++) {
		p += stride;
		const Vector3 *v = (const Vector3 *)p;
		if (v->x < min->x)
			min->x = v->x;
		if (v->y < min->y)
			min->y = v->y;
		if (v->z < min->z)
			min->z = v->z;
		if (v->x > max->x)
			max->x = v->x;
		if (v->y > max->y)
			max->y = v->y;
		if (v->z > max->z)
			max->z = v->z;
	}
	return D3D_OK;
}

HRESULT FHCreateMesh(unsigned int numFaces, unsigned int numVertices, unsigned int options,
                     const D3DVERTEXELEMENT9 *decl, DeviceEmu *device, DXMeshEmu **mesh)
{
	DXMeshEmu *m = DXMeshEmu::_alloc()->initWithParams(device, decl, numVertices, numFaces, options);
	if (m == 0)
		return (HRESULT)0x8876017c;
	*mesh = m;
	return D3D_OK;
}

// ---------------------------------------------------------------------------
// Images

static bool _isDXT(D3DFORMAT format)
{
	return format == D3DFMT_DXT1 || format == D3DFMT_DXT3 || format == D3DFMT_DXT5;
}

unsigned int FHGetSurfaceSize(unsigned int width, unsigned int height, D3DFORMAT format)
{
	if (_isDXT(format)) {
		unsigned int blocksX = width >> 2 ? width >> 2 : 1;
		unsigned int blocksY = height >> 2 ? height >> 2 : 1;
		return blocksX * blocksY * (format == D3DFMT_DXT1 ? 8 : 16);
	}
	return CBaseTexture::sizeOfPixelInFormat(format) * width * height;
}

// Copies a DDS file's levels into a texture of the same size and format.
HRESULT FHFillTextureWithData(const D3DXIMAGE_INFO *info, const void *dds, Texture2DEmu *texture)
{
	if (texture == 0)
		return D3DERR_INVALIDCALL;
	D3DFORMAT format = info->Format;
	unsigned int pitch;
	bool compressed;
	switch ((unsigned int)format) {
	case 0x14:
	case 0x15:
	case 0x16:
	case 0x17:
	case 0x18:
	case 0x19:
	case 0x1a:
	case 0x1c:
	case 0x1e:
	case 0x23:
	case 0x32:
	case 0x33: {
		const uint32_t *header = (const uint32_t *)dds;
		if (header[2] & 8)	// DDSD_PITCH
			pitch = header[5];
		else
			pitch = CBaseTexture::sizeOfPixelInFormat(format) * info->Width;
		compressed = false;
		break;
	}
	case D3DFMT_DXT1:
	case D3DFMT_DXT2:
	case D3DFMT_DXT3:
	case D3DFMT_DXT4:
	case D3DFMT_DXT5:
		pitch = 0;
		compressed = true;
		break;
	default:
		return D3DERR_INVALIDCALL;
	}

	unsigned int levels = texture->GetLevelCount();
	D3DSURFACE_DESC desc;
	texture->GetLevelDesc(0, &desc);
	if (info->MipLevels < levels)
		return D3DERR_INVALIDCALL;
	if (desc.Width != info->Width || desc.Height != info->Height || desc.Format != info->Format)
		return D3DERR_INVALIDCALL;

	unsigned int width = desc.Width, height = desc.Height;
	// After the magic and the header, whose size is its first field.
	const char *src = (const char *)dds + 4 + ((const uint32_t *)dds)[1];
	for (unsigned int level = 0; level < levels && level < info->MipLevels; level++) {
		size_t size;
		if (compressed) {
			size = FHGetSurfaceSize(width, height, format);
		} else {
			unsigned int rowPitch = pitch;
			if (info->ResourceType == D3DRTYPE_VOLUMETEXTURE)
				rowPitch = pitch / ((int)info->Depth < 1 ? 1 : info->Depth);
			size = (rowPitch >> level) * height;
		}
		D3DLOCKED_RECT locked;
		texture->LockRect(level, &locked, 0, D3DLOCK_DISCARD);
		if (locked.pBits)
			memcpy(locked.pBits, src, size);
		texture->UnlockRect(level);
		if (info->ResourceType == D3DRTYPE_VOLUMETEXTURE) {
			int depth = info->Depth >> level;
			size *= depth < 1 ? 1 : depth;
		}
		src += size;
		width = width >> 1 ? width >> 1 : 1;
		height = height >> 1 ? height >> 1 : 1;
	}
	return D3D_OK;
}

HRESULT FHGetImageInfoFromFileInMemory(const void *data, unsigned int size, D3DXIMAGE_INFO *info)
{
	HRESULT result = D3DERR_INVALIDCALL;
	LAutoreleasePool::addNewPoolToTop();
	ImageDecompressor *decompressor =
	    ImageDecompressor::decompressorForData(LData::dataWithBytesNoCopy((void *)data, size, false));
	if (decompressor) {
		decompressor->fillImageInfo(info);
		result = D3D_OK;
	}
	LAutoreleasePool::removeTopPool();
	return result;
}

// Only DDS files can be loaded straight into a texture.
HRESULT FHCreateTextureFromFileInMemory(DeviceEmu *device, const void *data, unsigned int size, unsigned int,
                                        unsigned int usage, D3DPOOL pool, Texture2DEmu **texture)
{
	D3DXIMAGE_INFO info;
	HRESULT hr = FHGetImageInfoFromFileInMemory(data, size, &info);
	if (hr < 0)
		return hr;
	if (info.ImageFileFormat != D3DXIFF_DDS)
		return D3DERR_INVALIDCALL;
	Texture2DEmu *created = 0;
	if (info.MipLevels == 0)
		info.MipLevels = 1;
	hr = device->CreateTexture(info.Width, info.Height, info.MipLevels, usage, info.Format, pool, &created);
	if (created && hr >= 0) {
		*texture = created;
		hr = FHFillTextureWithData(&info, data, created);
	}
	return hr;
}

// ---------------------------------------------------------------------------
// GLBugFixes

GLBugFixes::GLBugFixes() : CContextObject(10), _program(0) {}

void GLBugFixes::instantiateSelf()
{
	static const char kProgram[] = "!!ARBvp1.0\n"
	                               "MOV result.position, vertex.position;\n"
	                               "MOV result.texcoord[0], vertex.texcoord[0];\n"
	                               "MOV result.texcoord[1], vertex.texcoord[0];\n"
	                               "MOV result.texcoord[2], vertex.texcoord[0];\n"
	                               "MOV result.texcoord[3], vertex.texcoord[0];\n"
	                               "MOV result.texcoord[4], vertex.texcoord[0];\n"
	                               "MOV result.texcoord[5], vertex.texcoord[0];\n"
	                               "MOV result.texcoord[6], vertex.texcoord[0];\n"
	                               "MOV result.texcoord[7], vertex.texcoord[0];\n"
	                               "MOV result.fogcoord, vertex.fogcoord;\n"
	                               "END\n";
	glGenProgramsARB(1, &_program);
	glBindProgramARB(GL_VERTEX_PROGRAM_ARB, _program);
	glProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, sizeof kProgram - 1, kProgram);
}

void GLBugFixes::releaseSelf()
{
	glDeleteProgramsARB(1, &_program);
}

// Draws 32 culled triangles through the pass-through vertex program.
void GLBugFixes::drawVertexProgramTexcoordFix()
{
	glBindProgramARB(GL_VERTEX_PROGRAM_ARB, _program);
	glEnable(GL_VERTEX_PROGRAM_ARB);
	glFrontFace(GL_CW);
	glCullFace(GL_FRONT);
	glEnable(GL_CULL_FACE);
	glBegin(GL_TRIANGLES);
	for (int i = 0; i < 32; i++) {
		float x = (float)i * 0.01f;
		glTexCoord2f(0, 0);
		glFogCoordf(0);
		glVertex3f(x, 0, 0);
		glTexCoord2f(0, 1);
		glFogCoordf(0);
		glVertex3f(x, 0.01f, 0);
		glTexCoord2f(1, 1);
		glFogCoordf(0);
		glVertex3f(x + 0.01f, 0.01f, 0);
	}
	glEnd();
	glBindProgramARB(GL_VERTEX_PROGRAM_ARB, 0);
	glDisable(GL_VERTEX_PROGRAM_ARB);
	glDisable(GL_CULL_FACE);
}
