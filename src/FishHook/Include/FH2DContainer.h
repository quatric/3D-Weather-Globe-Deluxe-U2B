// C2DContainer: lines and triangles of 2D primitives collected in patches
// and drawn with the 2DPrimitives profile.

#ifndef FH2DCONTAINER_H
#define FH2DCONTAINER_H

#include "FHModels.h"

class C2DContainer : public CBuffersOwner, public IBuffersOwnerDataSource, public IVertexBufferDataSource {
	LF_DECLARE_CLASS(C2DContainer)
public:
	struct SPatchInfo {
		unsigned int start;	// the first vertex
		D3DPRIMITIVETYPE type;
		unsigned int count;	// primitives
	};

	// Adds a patch of one primitive type for its lifetime.
	class StPatch {
	public:
		struct SPoint {
			float x, y, z;
			unsigned int color;
		};
		StPatch(C2DContainer *container, D3DPRIMITIVETYPE type);
		// Line lists and strips.
		void addLine(const SPoint *a, const SPoint *b);
		// Line strips.
		void lineTo(const SPoint *p);
		// Triangle lists, strips and fans.
		void addTriangle(const SPoint *a, const SPoint *b, const SPoint *c);
		// Triangle strips and fans.
		void triangleTo(const SPoint *p);

		C2DContainer *_container;
		SPatchInfo *_patch;
	};

	C2DContainer();
	virtual void dealloc();
	virtual LClass *modelDataClass();
	virtual bool evalVisibilityParams(CDrawableModelData *, Vector3 *, float *) { return true; }
	virtual void drawSubset(CDrawableModelData *data, long subset, long index);
	virtual D3DPRIMITIVETYPE primitiveTypeForBufOwner(CBuffersOwner *) { return D3DPT_POINTLIST; }
	virtual CVertexBuffer *vertexBufferForBufOwner(int stream, CBuffersOwner *owner);
	virtual CIndexBuffer *indexBufferForBufOwner(CBuffersOwner *) { return 0; }
	virtual unsigned int vertCountForBuffer(CVertexBuffer *vb);
	virtual CShader *renderShaderForBuffer(CVertexBuffer *) { return renderProfile()->_shader; }
	virtual unsigned int streamNumberForBuffer(CVertexBuffer *) { return 0; }
	virtual void fillBuffer(CVertexBuffer *vb);

	C2DContainer *initWithHolder(CHolder *holder);

	TArray<SPatchInfo> *_patches;
	LData *_data;	// the vertices
	CVertexBuffer *_vb;
};

class C2DContainerDrawableData : public ModelDataWithMatrix {
	LF_DECLARE_CLASS(C2DContainerDrawableData)
public:
	C2DContainerDrawableData() : _flag(true) {}
	virtual const void *_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer);
	virtual bool _isStageOn(CShaderStage *stage, unsigned char *on);

	bool _flag;	// clear: constant 5 is asked as 3
};

#endif
