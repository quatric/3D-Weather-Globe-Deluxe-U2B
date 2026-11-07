// CClipingShaderStage: gives the shaders the far clip value, set by the scene
// before it draws.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(CClipingShaderStage, LF_CLASSOF(CShaderStage))

float CClipingShaderStage::sFarClipValue[4] = {1.0f, 0.0f, 0.0f, 0.0f};

CClipingShaderStage::CClipingShaderStage()
{
	sFarClipValue[0] = 1.0f;
	sFarClipValue[1] = 0.0f;
}

const void *CClipingShaderStage::_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer)
{
	if (descr->id != 'fcvl')
		return CShaderStage::_dataPtrForConstant(descr, buffer);
	return sFarClipValue;
}

void CClipingShaderStage::setFarClipValue(float value)
{
	sFarClipValue[0] = 1.0f / value;
	sFarClipValue[1] = 1.0f / value - 1.0f;
}
