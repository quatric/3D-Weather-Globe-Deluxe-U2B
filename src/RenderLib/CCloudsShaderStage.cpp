// The clouds shader stage: blends the frames only with the high animation quality.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(CCloudsShaderStage, LF_CLASSOF(CShaderStage))

int CCloudsShaderStage::cloudsAnimationQuality = 0;

const int *CCloudsShaderStage::addressOfVariable(LString *name)
{
	if (!name->isEqualTo(&kEnvParamUseBlending))
		return CShaderStage::addressOfVariable(name);
	return &cloudsAnimationQuality;
}
