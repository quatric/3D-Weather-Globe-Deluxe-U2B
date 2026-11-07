// The flat map earth's shader stage.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(EarthMapShaderStage, LF_CLASSOF(CShaderStage))

const int *EarthMapShaderStage::addressOfVariable(LString *name)
{
	if (name->isEqualTo(&sDayNightVarName))
		return RenderOptions::dayNightIsEnabledFlagPtr();
	return CShaderStage::addressOfVariable(name);
}
