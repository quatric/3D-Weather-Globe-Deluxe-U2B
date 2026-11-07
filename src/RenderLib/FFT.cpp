// FFT: Danielson-Lanczos with the bit-reversal permutation first and the
// twiddle factors by recurrence.

#include "RenderLib.h"

FFT::FFT(unsigned int log2Width, unsigned int log2Height, SComplexF *data)
    : _log2Width(log2Width), _log2Height(log2Height), _enabled(true), _data(data), _column(0)
{
}

FFT::~FFT()
{
	delete[] _column;
	_column = 0;
}

void FFT::_perform_fft(unsigned int log2n, int direction, SComplexF *data)
{
	int n = 1 << log2n;
	int j = 0;
	for (int i = 0; i < n - 1; i++) {
		if (i < j) {
			SComplexF t = data[i];
			data[i] = data[j];
			data[j] = t;
		}
		int k = n;
		while ((k >>= 1) <= j)
			j -= k;
		j += k;
	}
	float c = -1.0f, s = 0.0f;
	int half = 1;
	for (unsigned int level = 0; level < log2n; level++) {
		float wr = 1.0f, wi = 0.0f;
		for (int m = 0; m < half; m++) {
			for (int i = m; i < n; i += half * 2) {
				SComplexF &a = data[i];
				SComplexF &b = data[i + half];
				float tr = wr * b.re - wi * b.im;
				float ti = b.im * wr + b.re * wi;
				b.re = a.re - tr;
				a.re = tr + a.re;
				b.im = a.im - ti;
				a.im = ti + a.im;
			}
			float t = s * wi;
			wi = wi * c + wr * s;
			wr = c * wr - t;
		}
		s = sqrtf((1.0f - c) * 0.5f);
		if (direction > 0)
			s = -s;
		c = sqrtf((c + 1.0f) * 0.5f);
		half *= 2;
	}
	if (direction > 0) {
		float scale = 1.0f / (float)n;
		for (int i = 0; i < n; i++) {
			data[i].re *= scale;
			data[i].im *= scale;
		}
	}
}

void FFT::_perform_fft_2d(unsigned int log2Width, unsigned int log2Height, int direction, SComplexF *data,
                          SComplexF *column)
{
	int width = 1 << log2Width;
	int height = 1 << log2Height;
	for (int y = 0; y < height; y++)
		_perform_fft(log2Width, direction, data + y * width);
	for (int x = 0; x < width; x++) {
		for (int y = 0; y < height; y++)
			column[y] = data[x + y * width];
		_perform_fft(log2Height, direction, column);
		for (int y = 0; y < height; y++)
			data[x + y * width] = column[y];
	}
}

void FFT::execute()
{
	if (!_enabled)
		return;
	if (_column == 0)
		_column = new SComplexF[1 << _log2Height];
	_perform_fft_2d(_log2Width, _log2Height, -1, _data, _column);
}
