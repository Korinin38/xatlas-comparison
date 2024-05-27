#ifdef __CLION_IDE__
#include <libgpu/opencl/cl/clion_defines.cl>
#endif

#line 6

//m_rowStride = (m_width + 63) >> 6;

bool canBlit(__global unsigned long *one,
			 __global unsigned long *other,
			 unsigned int w, unsigned int h,
			 unsigned int cw, unsigned int ch,
			 unsigned int offsetX, unsigned int offsetY) {
	unsigned int rowStride = (w + 63) >> 6;
	unsigned int otherRowStride = (cw + 63) >> 6;
	for (uint32_t y = 0; y < ch; y++) {
		const unsigned int thisY = y + offsetY;
		if (thisY >= h)
			continue;
		unsigned int x = 0;
		for (;;) {
			const unsigned int thisX = x + offsetX;
			if (thisX >= w)
				break;
			const unsigned int thisBlockShift = thisX % 64;
			const unsigned long thisBlock = one[(thisX >> 6) + thisY * rowStride] >> thisBlockShift;
			const unsigned int blockShift = x % 64;
			const unsigned long block = other[(x >> 6) + y * otherRowStride] >> blockShift;
			if ((thisBlock & block) != 0)
				return false;
			x += 64 - max(thisBlockShift, blockShift);
			if (x >= cw)
				break;
		}
	}
	return true;
}

__kernel void blit(__global unsigned long *atlas, __global unsigned long *chart,
				   const int w, const int h,
				   const int cw, const int ch,
				   __global unsigned char *results) {
	const unsigned int x = get_global_id(0);
	const unsigned int y = get_global_id(1);

	if (x + cw > w || y + ch > h)
		return;

	if (canBlit(atlas, chart, w, h, cw, ch, x, y)) {
		results[y * (w + 1) + x] = 1;
	}
}