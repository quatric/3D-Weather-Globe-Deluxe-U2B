// triangle: the original compiled in Jonathan Shewchuk's Triangle 1.6
// (REAL = float) for one call, CPolysDrawable::_triangulate's
// triangulate("QzpXY", ...): one polygon ring (its points and the segments
// joining them in order), no holes, no Steiner points, zero-based indices.
// This replaces it with ear clipping over the same triangulateio interface:
// the triangles cover the same ring, though they are not the constrained
// Delaunay ones (the caller subdivides long edges anyway).

#include "RenderLib.h"

namespace {

struct Ring {
	const float *pts;
	TArray<int> prev, next;

	double x(int i) const { return pts[i * 2]; }
	double y(int i) const { return pts[i * 2 + 1]; }
	// Twice the signed area of (a, b, c): positive when counter-clockwise.
	double cross(int a, int b, int c) const
	{
		return (x(b) - x(a)) * (y(c) - y(a)) - (y(b) - y(a)) * (x(c) - x(a));
	}
	bool inside(int a, int b, int c, int p) const
	{
		return cross(a, b, p) >= 0 && cross(b, c, p) >= 0 && cross(c, a, p) >= 0;
	}
	// Whether no other vertex of the ring is in the triangle at v; only
	// reflex vertices can be.
	bool isEar(int v) const
	{
		int a = prev[v], c = next[v];
		if (cross(a, v, c) <= 0)
			return false;
		for (int p = next[c]; p != a; p = next[p]) {
			if ((x(p) == x(a) && y(p) == y(a)) || (x(p) == x(v) && y(p) == y(v)) ||
			    (x(p) == x(c) && y(p) == y(c)))
				continue;
			if (cross(prev[p], p, next[p]) <= 0 && inside(a, v, c, p))
				return false;
		}
		return true;
	}
};

} // namespace

void triangulate(const char *, triangulateio *in, triangulateio *out, triangulateio *)
{
	int n = in->numberofpoints;
	out->numberofpoints = n;
	out->pointlist = (float *)malloc((n > 0 ? n : 1) * 2 * sizeof(float));
	memcpy(out->pointlist, in->pointlist, n * 2 * sizeof(float));
	out->trianglelist = 0;
	out->numberoftriangles = 0;
	out->numberofcorners = 3;
	if (n < 3)
		return;

	Ring ring;
	ring.pts = in->pointlist;
	for (int i = 0; i < n; i++) {
		ring.prev.addObject(0);
		ring.next.addObject(0);
	}
	// The segments give the ring's order; without them, the points' order.
	if (in->segmentlist && in->numberofsegments == n) {
		for (int s = 0; s < n; s++) {
			int a = in->segmentlist[s * 2], b = in->segmentlist[s * 2 + 1];
			ring.next[a] = b;
			ring.prev[b] = a;
		}
	} else {
		for (int i = 0; i < n; i++) {
			ring.next[i] = (i + 1) % n;
			ring.prev[(i + 1) % n] = i;
		}
	}
	// Walk it counter-clockwise.
	double area = 0;
	for (int k = 0, i = 0; k < n; k++, i = ring.next[i]) {
		int j = ring.next[i];
		area += ring.x(i) * ring.y(j) - ring.x(j) * ring.y(i);
	}
	if (area < 0)
		for (int i = 0; i < n; i++) {
			int t = ring.next[i];
			ring.next[i] = ring.prev[i];
			ring.prev[i] = t;
		}

	int *tris = (int *)malloc((n - 2) * 3 * sizeof(int));
	int count = 0;
	int left = n;
	int v = 0;
	int stalled = 0;	// vertices looked at since the last one went
	while (left > 3) {
		int a = ring.prev[v], c = ring.next[v];
		double turn = ring.cross(a, v, c);
		// Collinear and doubled points go without a triangle; ears go with
		// one; when a whole round finds neither (self-crossing rings),
		// a convex vertex goes anyway, and after a second round any.
		bool drop = turn == 0;
		bool ear = !drop && (stalled < left ? ring.isEar(v) : stalled < 2 * left ? turn > 0 : true);
		if (drop || ear) {
			if (ear) {
				tris[count * 3] = a;
				tris[count * 3 + 1] = v;
				tris[count * 3 + 2] = c;
				count++;
			}
			ring.next[a] = c;
			ring.prev[c] = a;
			left--;
			stalled = 0;
			v = c;
		} else {
			stalled++;
			v = c;
		}
	}
	int a = ring.prev[v], c = ring.next[v];
	if (ring.cross(a, v, c) != 0) {
		tris[count * 3] = a;
		tris[count * 3 + 1] = v;
		tris[count * 3 + 2] = c;
		count++;
	}
	if (count == 0) {
		free(tris);
		return;
	}
	out->trianglelist = tris;
	out->numberoftriangles = count;
}
