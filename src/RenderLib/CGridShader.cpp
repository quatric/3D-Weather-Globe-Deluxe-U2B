// The grid's shader.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(CGridShader, LF_CLASSOF(CShader))

// The original read a caps field that is always 1: lines are antialiased.
const int *CGridShader::addressOfVariable(LString *name)
{
	static const int lineAntialias = 1;
	if (name->isEqualTo(&sLineAntialias))
		return &lineAntialias;
	return CShader::addressOfVariable(name);
}
