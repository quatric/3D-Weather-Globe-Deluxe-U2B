// QSORT
//
// LF::QSORT: the classic quicksort with a selection sort below 9 elements
// (tqsort<>), kept as is so equal elements end up where the original put them.

#include "LContainers.h"

namespace LF {

static void _swap(char *a, char *b, unsigned int size)
{
	if (a != b)
		for (unsigned int i = 0; i < size; i++) {
			char t = a[i];
			a[i] = b[i];
			b[i] = t;
		}
}

static void _shortsort(char *lo, char *hi, unsigned int size, CArrayComparator cmp, const void *ctx)
{
	for (; lo < hi; hi -= size) {
		char *max = lo;
		for (char *p = lo + size; p <= hi; p += size)
			if (cmp(p, max, ctx) > 0)
				max = p;
		_swap(max, hi, size);
	}
}

void QSORT::sortInContext(void *base, unsigned int count, unsigned int size, const void *ctx,
                          CArrayComparator cmp)
{
	if (count < 2 || size == 0)
		return;
	char *loStack[30], *hiStack[30];
	int sp = 0;
	char *lo = (char *)base;
	char *hi = (char *)base + (count - 1) * size;

	for (;;) {
		unsigned int n = (unsigned int)(hi - lo) / size + 1;
		if (n <= 8) {
			_shortsort(lo, hi, size, cmp, ctx);
		} else {
			_swap(lo + (n / 2) * size, lo, size);
			char *l = lo, *h = hi + size;
			for (;;) {
				do
					l += size;
				while (l <= hi && cmp(l, lo, ctx) <= 0);
				do
					h -= size;
				while (h > lo && cmp(h, lo, ctx) >= 0);
				if (h < l)
					break;
				_swap(l, h, size);
			}
			_swap(lo, h, size);

			if (h - 1 - lo >= hi - l) {
				if (lo + size < h) {
					loStack[sp] = lo;
					hiStack[sp] = h - size;
					sp++;
				}
				if (l < hi) {
					lo = l;
					continue;
				}
			} else {
				if (l < hi) {
					loStack[sp] = l;
					hiStack[sp] = hi;
					sp++;
				}
				if (lo + size < h) {
					hi = h - size;
					continue;
				}
			}
		}
		if (--sp < 0)
			return;
		lo = loStack[sp];
		hi = hiStack[sp];
	}
}

} // namespace LF
