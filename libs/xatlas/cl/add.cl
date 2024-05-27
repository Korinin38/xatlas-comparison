#ifdef __CLION_IDE__
#include <libgpu/opencl/cl/clion_defines.cl>
#endif

#line 6

__kernel void add(__global unsigned long *atlas, __global const unsigned long *chart,
				   const int offset_x, const int offset_y,
				   const int w, const int h,
				   const int cw, const int ch) {
	const unsigned int x = get_global_id(0);
	const unsigned int y = get_global_id(1);

	if (x > w || y > h) {
		return;
	}

	bool result = true;

//	const uint32_t index = (x >> 6) + y * m_rowStride;
//	m_data[index] |= UINT64_C(1) << (uint64_t(x) & UINT64_C(63));
	unsigned int rowStride = (w + 63) >> 6;
	unsigned int otherRowStride = (cw + 63) >> 6;
	for (size_t y = 0; y < ch; y++)
	{
		const unsigned int thisY = y + offset_y;
		if (thisY >= size_y)
			continue;
		unsigned int x = 0;
	}
}