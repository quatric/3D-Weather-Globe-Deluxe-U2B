// LayoutManager: places caption rectangles near their points without
// overlapping, on a grid of cells. Nothing in the application calls it (the
// captions are laid out by the billboard system), but it is part of RenderLib.
//
//   LMCreate(radius, markerSize, hAlign, vAlign)
//   LMBeginBlock / LMAddBasePoint... / LMEndBlock / LMAddRectangle...
//   LMCalculateLayout, then LMGetRectangle for each rectangle; LMDestroy.

#ifndef LAYOUTMANAGER_H
#define LAYOUTMANAGER_H

#include "LString.h"

struct SLayoutGrid {
	char *cells;	// the number of what covers each cell, 0 when free
	LF::LRect bounds;
	float columns;
	float rows;
};

struct SLayoutPoint {
	LF::LPoint point;
	LF::LRect bounds;	// of its rectangles
	float cell;	// the cell of its area's corner
	float size;	// its area's side, in cells
};

struct SLayoutRectangle {
	LF::LRect rect;
	int point;	// the index of its base point
};

struct __opaqueLMContext {
	SLayoutGrid *grid;
	int pointsCount;
	int pointsCapacity;
	SLayoutPoint *points;
	int rectanglesCapacity;
	int rectanglesCount;
	SLayoutRectangle *rectangles;
	float scale;	// of the coordinates, set by the first point
	float hAlign;	// where the point is in its area, as a part of the radius
	float vAlign;
	float radius;	// of the points' areas
	float markerSize;	// of the points' markers
	float cellSize;
	unsigned int flags;	// kLMTerminate
};
typedef __opaqueLMContext *LMContextRef;

enum { kLMTerminate = 1 };

SLayoutRectangle MakeSLayoutRectangle(LF::LRect rect, int point);
SLayoutPoint MakeLayoutPoint(LF::LPoint point, float size, LF::LRect bounds);
float CalculateScaleForCoordinates(LF::LPoint point);

LMContextRef LMCreate(float radius, float markerSize, float hAlign, float vAlign);
void LMDestroy(LMContextRef context);
bool LMBeginBlock(LMContextRef context);
int LMAddBasePoint(LMContextRef context, LF::LPoint point);
bool LMEndBlock(LMContextRef context);
int LMAddRectangle(LMContextRef context, LF::LRect rect, int point);
bool LMCalculateLayout(LMContextRef context);
// Stops LMCalculateLayout (from another thread).
bool LMTerminateLayoutCalculation(LMContextRef context);
bool LMGetRectangle(LMContextRef context, int index, LF::LRect *rect);

#endif
