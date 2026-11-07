// A tile of the flat map: its place and its textures.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(FlatEntryObjectData, LF_CLASSOF(ModelDataWithMatrix))

bool FlatEntryObjectData::_getTextureForSource(BaseTextureEmu **texture, int source)
{
	if (source == 0)
		*texture = _loader->textureAtLevel(_texture.textureID, 0);
	else if (source == 1)
		*texture = _loader->nightGlowsTextureAtLevel(_glows.textureID);
	else
		return CDrawableModelData::_getTextureForSource(texture, source);
	return true;
}

void FlatEntryObjectData::setDrawParameters(const STextureID &id, CTextureLoader *loader, float offsetX, float lights)
{
	_loader = loader;
	_alpha = lights;
	// A level's map is 2 x 1, of 2n x n patches.
	int rows = textureRowsAtLevel(id.level);
	float size = 1.0f / (float)rows;
	_matrix._22 = size;
	_matrix._11 = size * 0.5f;
	_matrix._41 = (float)id.x * size * 0.5f + offsetX;
	_matrix._42 = (float)id.y * size;
	// The textures start at the date line.
	STextureID shifted = id;
	shifted.x = (short)(id.x + rows);
	if (rows * 2 <= shifted.x)
		shifted.x = (short)(shifted.x - rows * 2);
	shifted.x = (short)(shifted.x / STextureID::columnsMergedInRow(id.level, id.y));
	_texture.adjust(id, shifted, loader->requestTextureBeforeDraw(shifted), 1);
	_glows.adjust(id, shifted, CTextureLoader::requestGlowsTextureBeforeDraw(shifted), 1);
}

const void *FlatEntryObjectData::_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer)
{
	switch (descr->id) {
	case 'TTro':
		_glows.fillVSScaleOffsConstant((Vector4 *)buffer);
		return buffer;
	case 'TTrn':
		_texture.fillVSScaleOffsConstant((Vector4 *)buffer);
		return buffer;
	case 'TTrp':
		*(Color *)buffer = Color(0.5f, 1.0f, 0.2f, 0.0f) * _alpha;
		return buffer;
	default:
		return CDrawableModelData::_dataPtrForConstant(descr, buffer);
	}
}
