// Shapes: the outlines of countries, states, counties and time zones, read
// from index BLOBs, found by point through lookup grids, and drawn with
// RenderLib's drawables.

#ifndef AMSHAPES_H
#define AMSHAPES_H

#include "AMDatabase.h"

class CShapeDrawable;

// The model data of a shape: its highlight color, constant 'ShHg'.
class ShapeModelData : public ModelDataWithMatrix {
	LF_DECLARE_CLASS(ShapeModelData)
public:
	enum { kHighlightColorConstID = 0x53684867 };
	static LString *kUsageHitShapeHighlight;
	static LString *kUsageAllShapesHighlight;

	ShapeModelData();
	virtual void dealloc();
	virtual ShapeModelData *getOwnerLObject() { return this; }
	virtual const void *_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer);
	virtual void setupWithUsageKey(const LString *key) {}

	Color _color;
	CShapeDrawable *_drawable;	// not retained
	void *_buffer;	// new[]
};

// Draws a shape container for a projection; the models come from
// _classForModel() and draw with the buffers this provides.
class CShapeDrawable : public LObject, public IBuffersOwnerDataSource {
	LF_DECLARE_ABSTRACT_CLASS(CShapeDrawable)
public:
	CShapeDrawable();
	virtual void dealloc();
	virtual CShapeDrawable *initWithParams(int mode, CShapeContainer *container, CProjection *projection);
	// The model datas of the drawable for the usage key, made when needed
	// (*created is then set).
	virtual LArray *drawableDatas(int index, CHolder *holder, const LString *usageKey, unsigned char *created);
	virtual LString *_renderProfileFile() = 0;
	virtual int _modelsCount() = 0;
	virtual LClass *_classForModel(unsigned int index);
	virtual void _modelWasCreated(CBuffersOwner *model, unsigned int index) {}

	CRenderProfile *renderProfile(CHolder *holder);

	CShapeContainer *_container;
	CProjection *_projection;
	int _mode;
	LArray *_models;
	LMap *_datasByUsage;
	CRenderProfile *_profile;
};

// The shapes of one object, from its index BLOB.
class CShapeContainer : public LObject {
	LF_DECLARE_CLASS(CShapeContainer)
public:
	CShapeContainer();
	virtual void dealloc();
	virtual CShapeContainer *getOwnerLObject() { return this; }
	virtual CShapeContainer *initWithIndexWrapper(IndexBuffersBLOB *indexes, const LString *objectUID,
	                                              const LString *shapeID);
	virtual bool hitTest(LPoint geoPoint);
	virtual LRect allPathsBoundsGeoRect();
	virtual void invalidateDrawables();
	virtual CShapeDrawable *drawableForProjection(CProjection *projection, int mode, float scale);
	virtual LClass *drawableClass();
	virtual bool _loadShapes();

	QuadTreeContainer *quadTree(unsigned char east);
	void _loadShapesIfNeeded();
	LString *objectUID() const { return _objectUID; }
	LString *shapeID() const { return _shapeID; }

	IndexBuffersBLOB *_indexes;
	LString *_objectUID;
	LString *_shapeID;
	CShapeDrawable *_drawable;
	int _drawableMode;
	bool _loaded;
};

// The shape containers by shape ID and object UID, and the lookup grids.
class CShapeManager : public LObject {
	LF_DECLARE_CLASS(CShapeManager)
public:
	static LString *kShapeIDStates;
	static LString *kShapeIDCounties;
	static LString *kShapeIDCountries;
	static LString *kShapeIDTimezones;

	static TLArray<LString *> *__registeredShapes;
	static LMap *__shapeIDToPublicNameRemap;
	static LMap *__shapeIDToColorRemap;
	static LMap *__shapeIDToFillColorRemap;
	static LMap *__shapeIDToGridNameRemap;
	static LMap *__shapeIDToClassRemap;
	static LMap *__shapeIDToGeotypeNamesRemap;

	CShapeManager();
	virtual void initAsSingleton(void *param);
	virtual void dealloc();
	virtual CShapeManager *getOwnerLObject() { return this; }
	virtual intptr_t _onDBUpdateFinished(char *args);

	void invalAllDrawables();
	static bool highlightColorForShapeID(const LString *shapeID, Color *color);
	static bool highlightFillColorForShapeID(const LString *shapeID, Color *color);
	static LString *_dbGridNameForShapeWithID(const LString *shapeID);
	static LString *publicNameFromShapeID(const LString *shapeID);
	static LClass *_shapeClassForShapeID(const LString *shapeID);
	static TLArray<LString *> *registeredShapeIDs();
	static TLArray<LString *> *geotypesForShapeID(const LString *shapeID);
	// Retained.
	CShapeContainer *_createShapeContainer(const LString *objectUID, const LString *shapeID);
	LookupGridBLOB *lookupGridByShapeID(const LString *shapeID);
	CShapeContainer *shapeForObject(const LString *objectUID, const LString *shapeID);
	// The smallest shape at the point.
	LString *objectUIDByHit(LPoint geoPoint, const LString *shapeID);
	LArray *allShapesWithShapeID(const LString *shapeID);

	CSharableResource _lock;
	LMap *_lookupGrids;
	LMap *_shapes;
};

#endif
