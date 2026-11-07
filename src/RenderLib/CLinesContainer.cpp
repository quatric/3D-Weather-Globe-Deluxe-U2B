// CLinesContainer: a shape of lines. The original declares it but never
// loads one; its dealloc didn't call the base class's (fixed here).

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(CLinesContainer, LF_CLASSOF(CShapeContainer))
