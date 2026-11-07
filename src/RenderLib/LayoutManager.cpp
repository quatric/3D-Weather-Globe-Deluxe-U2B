// LayoutManager: caption placement on a grid. The points' areas are marked on
// the grid first; then each rectangle is checked at a few cells of its
// outline and middle, and when it covers something it is moved, row by row
// through its point's area, to the place with the most free cells.

#include "LayoutManager.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

using namespace LF;

SLayoutRectangle MakeSLayoutRectangle(LRect rect, int point)
{
	SLayoutRectangle r = { rect, point };
	return r;
}

// The size isn't kept (LMEndBlock works it out).
SLayoutPoint MakeLayoutPoint(LPoint point, float, LRect bounds)
{
	SLayoutPoint p;
	memset(&p, 0, sizeof p);
	p.point = point;
	p.bounds = bounds;
	return p;
}

// The power of 10 that makes the coordinates whole numbers of at least 20.
float CalculateScaleForCoordinates(LPoint point)
{
	float value = fabsf(point.x);
	if (value == 0)
		value = fabsf(point.y);
	if (value == 0)
		return 1;
	float scale = 1;
	while ((int)(scale * value) < 20)
		scale *= 10;
	return scale;
}

static void _freeBlock(LMContextRef context)
{
	if (context->grid) {
		if (context->grid->cells)
			free(context->grid->cells);
		free(context->grid);
		context->grid = 0;
	}
	if (context->points) {
		free(context->points);
		context->points = 0;
	}
	if (context->rectangles) {
		free(context->rectangles);
		context->rectangles = 0;
	}
}

void LMDestroy(LMContextRef context)
{
	if (!context)
		return;
	_freeBlock(context);
	free(context);
}

bool LMBeginBlock(LMContextRef context)
{
	if (context)
		_freeBlock(context);
	return true;
}

// The alignments are within -1.7...1.7 (0 isn't allowed: 1).
LMContextRef LMCreate(float radius, float markerSize, float hAlign, float vAlign)
{
	LMContextRef context = (LMContextRef)calloc(1, sizeof(__opaqueLMContext));
	context->radius = radius;
	if (hAlign == 0 || hAlign > 1.7f || hAlign < -1.7f)
		hAlign = 1;
	if (vAlign == 0 || vAlign > 1.7f || vAlign < -1.7f)
		vAlign = 1;
	context->scale = -1;
	context->markerSize = markerSize;
	context->vAlign = vAlign;
	context->hAlign = hAlign;
	return context;
}

int LMAddBasePoint(LMContextRef context, LPoint point)
{
	if (!context->points) {
		context->pointsCount = 0;
		context->pointsCapacity = 10;
		context->points = (SLayoutPoint *)malloc(10 * sizeof(SLayoutPoint));
	}
	if (context->scale < 0) {
		context->scale = 0.25f;
		context->radius *= 0.25f;
	}
	float scale = context->scale;
	int index = context->pointsCount;
	if (context->pointsCapacity <= index) {
		context->pointsCapacity += 10;
		context->points =
		    (SLayoutPoint *)realloc(context->points, context->pointsCapacity * sizeof(SLayoutPoint));
	}
	SLayoutPoint *p = &context->points[index];
	p->point = LMakePoint(point.x * scale, point.y * scale);
	p->bounds = LZeroRect;
	p->cell = 0;
	p->size = 0;
	context->pointsCount++;
	return index;
}

// Makes the grid: over the points' areas.
bool LMEndBlock(LMContextRef context)
{
	SLayoutPoint *points = context->points;
	if (!points)
		return true;
	float cellSize = context->cellSize;
	float side = context->radius + context->radius;
	float dx = context->radius * context->hAlign;
	float dy = context->radius * context->vAlign;
	LRect bounds = LZeroRect;
	for (int i = 0; i < context->pointsCount; i++) {
		LRect area = LMakeRect(points[i].point.x - dx, points[i].point.y - dy, side, side);
		bounds = LUnionRect(bounds, area);
		points[i].size = (float)(int)(side / cellSize);
	}
	SLayoutGrid *grid = (SLayoutGrid *)malloc(sizeof(SLayoutGrid));
	context->grid = grid;
	grid->bounds = bounds;
	int columns = (int)(bounds.size.width / cellSize);
	int rows = (int)(bounds.size.height / cellSize);
	grid->columns = (float)columns;
	grid->rows = (float)rows;
	grid->cells = (char *)malloc(rows * columns);
	for (int i = 0; i < context->pointsCount; i++) {
		int cell = (int)((points[i].point.x - dx - bounds.origin.x) / cellSize) +
		           (int)((points[i].point.y - dy - bounds.origin.y) / cellSize) * columns;
		points[i].cell = cell < 0 ? 0 : (float)cell;
	}
	return true;
}

int LMAddRectangle(LMContextRef context, LRect rect, int point)
{
	if (!context->rectangles) {
		context->rectanglesCount = 0;
		context->rectanglesCapacity = 10;
		context->rectangles = (SLayoutRectangle *)malloc(10 * sizeof(SLayoutRectangle));
	}
	float scale = context->scale;
	float height = rect.size.height * scale;
	// The cells are a quarter of the first rectangle's height.
	if (context->cellSize <= 0) {
		float cell = ceilf(height * 0.25f);
		context->cellSize = cell <= 1 ? 1 : cell;
	}
	int index = context->rectanglesCount;
	if (context->rectanglesCapacity <= index) {
		context->rectanglesCapacity += 30;
		context->rectangles = (SLayoutRectangle *)realloc(context->rectangles,
		                                                  context->rectanglesCapacity * sizeof(SLayoutRectangle));
	}
	SLayoutRectangle *r = &context->rectangles[index];
	r->rect = LMakeRect(rect.origin.x * scale, rect.origin.y * scale, rect.size.width * scale, height);
	r->point = point;
	context->rectanglesCount++;
	SLayoutPoint *p = &context->points[point];
	p->bounds = LUnionRect(p->bounds, r->rect);
	return index;
}

bool LMTerminateLayoutCalculation(LMContextRef context)
{
	context->flags |= kLMTerminate;
	return true;
}

bool LMGetRectangle(LMContextRef context, int index, LRect *rect)
{
	float scale = context->scale;
	const LRect &r = context->rectangles[index].rect;
	*rect = LMakeRect(r.origin.x / scale, r.origin.y / scale, r.size.width / scale, r.size.height / scale);
	return true;
}

bool LMCalculateLayout(LMContextRef context)
{
	SLayoutPoint *points = context->points;
	SLayoutRectangle *rect = context->rectangles;
	if (!points || !rect)
		return true;
	SLayoutGrid *grid = context->grid;
	float ox = grid->bounds.origin.x;
	float oy = grid->bounds.origin.y;
	int columns = (int)grid->columns;
	int rows = (int)grid->rows;
	char *cells = grid->cells;
	if (columns == 0 || rows == 0)
		return true;
	float cellSize = context->cellSize;
	memset(cells, 0, columns * rows);

	// The markers, numbered from 1.
	int span = (int)((context->markerSize / cellSize) * context->scale);
	int half = span / 2;
	int mark = 0;
	for (int i = 0; i < context->pointsCount; i++) {
		int x = (int)((points[i].point.x - ox) / cellSize - (float)half);
		if (x < 0)
			x = 0;
		if (x > columns - 1)
			x = columns - 1;
		int y = (int)((points[i].point.y - oy) / cellSize);
		if (y < 0)
			y = 0;
		if (y > rows - 1)
			y = rows - 1;
		mark = i + 1;
		int width = span < columns - x ? span : columns - x;
		// The original skipped row 0 and didn't stop at the last row.
		for (int row = y - half; row <= y + half; row++)
			if (row > 0 && row < rows)
				memset(cells + columns * row + x, mark, width);
	}

	int rectanglesCount = context->rectanglesCount;
	bool terminated = (context->flags & kLMTerminate) != 0;
	for (int j = 0; !terminated && j < rectanglesCount; j++, rect++) {
		mark++;
		terminated = (context->flags & kLMTerminate) != 0;
		int width = (int)(rect->rect.size.width / cellSize);
		int height = (int)(rect->rect.size.height / cellSize);
		int x = (int)((rect->rect.origin.x - ox) / cellSize);
		int y = (int)((rect->rect.origin.y - oy) / cellSize);
		if (y > rows - 1)
			y = rows - 1;
		if (x > columns - 1)
			x = columns - 1;
		int w = columns - (width + x) < 0 ? columns - x - 1 : width;
		int h = rows - (height + y) < 0 ? rows - y - 1 : height;

		// 10 samples, the middle counting twice.
		int top = columns * y + x;
		int bottom = (h - 1 + y) * columns + x;
		int middle = (h / 2 + y) * columns + x;
		int freeCells = 2 * (cells[middle + (w / 3) * 2] == 0) + (cells[top] == 0) + (cells[top + w / 2] == 0) +
		           (cells[top + w - 1] == 0) + (cells[bottom] == 0) + (cells[bottom + w / 2] == 0) +
		           (cells[bottom + w] == 0) + (cells[bottom + w / 3] == 0) + (cells[middle + (w * 3) / 5] == 0);
		bool moved = false;
		if (freeCells < 10 || cells[middle + w / 2] != 0) {
			moved = true;
			int step = (int)((float)width * 0.5f);
			if (step < 2)
				step = 2;
			const SLayoutPoint &base = points[rect->point];
			int areaCell = (int)base.cell;
			int areaSize = (int)base.size;
			int ax = areaCell % columns;
			int ay = areaCell / columns;
			int start = columns * ay + ax;
			int xEnd = ax + areaSize;
			int yEnd = areaSize + ay;
			int end = xEnd + 1 + columns * yEnd;
			int best = -1;
			int bestRow = y, bestColumn = x;
			bool found = false;
			// Each row of the area from right to left.
			for (int pos = start; !terminated && pos < end; pos -= step) {
				if (pos > end - 1)
					pos = end - 1;
				if (pos < start)
					pos = start;
				int row = pos / columns;
				int column = pos % columns;
				if (column <= ax) {
					start = ax + (row + 1) * columns;
					pos = xEnd - 1 + (row + 1) * columns + step;
					continue;
				}
				if (yEnd <= row)
					break;
				if (cells[pos] != 0)
					continue;
				int cw = width < columns - column ? width : columns - column;
				int last = cw - 1;
				int ch = height < rows - row ? height : rows - row;
				int half2 = last / 2;
				int bottom2 = (ch - 1 + row) * columns + column;
				int middle2 = ((ch - 1) / 2 + row) * columns + column;
				int count = (cells[pos + half2] == 0) + (cells[pos + last - 1] == 0) + (cells[pos + last / 3] == 0) +
				            (cells[pos + (last / 3) * 2] == 0) + (cells[bottom2] == 0) +
				            (cells[bottom2 + half2] == 0) + (cells[bottom2 + last - 1] == 0) +
				            (cells[bottom2 + last / 4] == 0) + (cells[middle2 + last] == 0);
				if (cells[middle2 + half2] == 0 && count > 8) {
					x = column;
					y = row;
					found = true;
					break;
				}
				if (best < count) {
					best = count;
					bestRow = row;
					bestColumn = column;
				}
				pos -= half2 > 1 ? half2 : 2;
			}
			if (!found) {
				x = bestColumn;
				y = bestRow;
			}
		}
		int fill = width;
		if (columns - (x + width) + 1 < 0)
			fill = columns - x;
		char *p = cells + y * columns + x;
		for (int row = y; row < rows && row < y + height; row++, p += columns)
			memset(p, mark, fill);
		if (moved) {
			rect->rect.origin.y = (float)y * cellSize + oy;
			rect->rect.origin.x = (float)x * cellSize + ox;
		}
	}
	return true;
}
