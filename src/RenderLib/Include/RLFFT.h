// FFT: an in-place radix-2 complex FFT of a 2^m x 2^n grid (rows, then
// columns through a scratch column).

#ifndef RLFFT_H
#define RLFFT_H

class FFT {
public:
	struct SComplexF {
		float re, im;
	};

	static SComplexF *complexf_malloc(unsigned int count) { return new SComplexF[count]; }
	static void complexf_free(SComplexF *data) { delete[] data; }

	// A grid of 2^log2Width x 2^log2Height values, not owned.
	FFT(unsigned int log2Width, unsigned int log2Height, SComplexF *data);
	virtual ~FFT();
	// The forward transform of the data.
	void execute();

	// `direction` > 0: the inverse, scaled by 1/n.
	static void _perform_fft(unsigned int log2n, int direction, SComplexF *data);
	static void _perform_fft_2d(unsigned int log2Width, unsigned int log2Height, int direction, SComplexF *data,
	                            SComplexF *column);

	unsigned int _log2Width;
	unsigned int _log2Height;
	bool _enabled;
	SComplexF *_data;
	SComplexF *_column;	// 2^log2Height scratch values
};

#endif
