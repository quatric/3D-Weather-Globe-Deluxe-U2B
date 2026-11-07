// The globe earth's shader stage.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(EarthGlobeMapShaderStage, LF_CLASSOF(CShaderStage))

const int *EarthGlobeMapShaderStage::addressOfVariable(LString *name)
{
	if (name->isEqualTo(&_sDayNightVarName))
		return RenderOptions::dayNightIsEnabledFlagPtr();
	if (name->isEqualTo(&_sOceanTransparencyVarName))
		return Preferences::oceanTransparencyIsEnabledFlagPtr();
	return CShaderStage::addressOfVariable(name);
}
