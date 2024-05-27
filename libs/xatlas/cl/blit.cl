#ifdef __CLION_IDE__

#include <libgpu/opencl/cl/clion_defines.cl>

#endif
#ifndef CUDA_SUPPORT
#undef STATIC_KEYWORD
#define STATIC_KEYWORD
#endif
#line 11

#define UINT16_MAX 65535

STATIC_KEYWORD bool canBlit(__global const unsigned long *atlas,
							__global const unsigned long *chart,
							const unsigned int offset_x,
							const unsigned int offset_y,
							__global const unsigned int *atlas_size,
							__global const unsigned int *chart_size
//							,__global const unsigned int  *best
)
{
	unsigned int atlasRowStride = (atlas_size[0] + 63) >> 6;
	unsigned int chartRowStride = (chart_size[0] + 63) >> 6;
	for (size_t y = 0; y < chart_size[1]; y++) {
//		{
//			// WARNING: RESOLUTION IS LIMITED BY 65535 to make the operation atomic (see 9203458923756982)
//			unsigned int best_cpy = *best;
//			unsigned int b_x = best_cpy >> 16;
//			unsigned int b_y = best_cpy & UINT16_MAX;
//			if (offset_x > b_x && offset_y > b_y)
//				return false;
//		}
		const unsigned int thisY = y + offset_y;
		if (thisY >= atlas_size[1])
			continue;
		unsigned int x = 0;
		for (;;) {
			const unsigned int thisX = x + offset_x;
			if (thisX >= atlas_size[0])
				break;
			const unsigned int thisBlockShift = thisX % 64;
			const unsigned long thisBlock = atlas[(thisX >> 6) + thisY * atlasRowStride] >> thisBlockShift;
			const unsigned int blockShift = x % 64;
			const unsigned long block = chart[(x >> 6) + y * chartRowStride] >> blockShift;
			if ((thisBlock & block) != 0)
				return false;
			x += 64 - max(thisBlockShift, blockShift);
			if (x >= chart_size[0])
				break;
		}
	}
	return true;
}

STATIC_KEYWORD __global const unsigned int *getChartSize(__global const unsigned int *chartSizes,
														 const int coarse_level,
														 const int rate,
														 const unsigned int offset_x,
														 const unsigned int offset_y)
{
	unsigned int ptr_offset = 0;
	for (int cur_level = 0; cur_level <= coarse_level; ++cur_level) {
		int cur_level_rate = 1;
		for (int j = 0; j < cur_level; ++j)
			cur_level_rate *= rate;
		for (int y = 0; y < cur_level_rate; ++y) {
			for (int x = 0; x < cur_level_rate; ++x) {
				if (cur_level == coarse_level && x == offset_x % cur_level_rate && y == offset_y % cur_level_rate) {
					return chartSizes + ptr_offset;
				} else {
					ptr_offset += 2;
				}
			}
		}
	}
	// should be unreachable
	printf("!UNREACHABLE CODE REACHED!\n");
}

STATIC_KEYWORD __global const unsigned long *getChart(__global const unsigned long *charts,
													  __global const unsigned int *chartSizes,
													  const int coarse_level,
													  const int rate,
													  const unsigned int offset_x,
													  const unsigned int offset_y)
{
	unsigned int ptr_chart_offset = 0;
	unsigned int ptr_size_offset = 0;

	for (int cur_level = 0; cur_level <= coarse_level; ++cur_level) {
		int cur_level_rate = 1;
		for (int j = 0; j < cur_level; ++j)
			cur_level_rate *= rate;
		for (int y = 0; y < cur_level_rate; ++y) {
			for (int x = 0; x < cur_level_rate; ++x) {
				if (cur_level == coarse_level && x == (offset_x % cur_level_rate) && y == (offset_y % cur_level_rate)) {
					return charts + ptr_chart_offset;
				}
				__global const unsigned int *size = chartSizes + ptr_size_offset;
				unsigned int rowStride = (size[0] + 63) >> 6;
				ptr_chart_offset += rowStride * size[1];
				ptr_size_offset += 2;
			}
		}
	}
	// should be unreachable
	printf("!UNREACHABLE CODE REACHED!\n");
}

STATIC_KEYWORD __global const unsigned int *getAtlasSize(__global const unsigned int *atlasSizes,
														 const int coarse_level)
{
	return atlasSizes + coarse_level * 2;
}

STATIC_KEYWORD __global const unsigned long *getAtlas(__global const unsigned long *atlases,
													  __global const unsigned int *atlasSizes,
													  const int coarse_level)
{
	unsigned int ptr_offset = 0;

	for (int cur_level = 0; cur_level <= coarse_level; ++cur_level) {
		if (cur_level == coarse_level) {
			return atlases + ptr_offset;
		}
		__global const unsigned int *size = getAtlasSize(atlasSizes, cur_level);
		unsigned int rowStride = (size[0] + 63) >> 6;
		ptr_offset += rowStride * size[1];
	}
	// should be unreachable
	printf("!UNREACHABLE CODE REACHED!\n");
}

//STATIC_KEYWORD bool iterateCanBlit(__global const unsigned long *atlases,
STATIC_KEYWORD bool iterateCanBlit(__global const unsigned long *atlases,
								   __global const unsigned long *charts,
								   const int level,
								   const int rate,
								   __global const unsigned int *atlasSizes,
								   __global const unsigned int *chartSizes,
								   const unsigned int offset_x,
								   const unsigned int offset_y
//								   ,__global unsigned int *best
)
{
	int coarse_reduction = 1;
	for (int i = 0; i < level; ++i) {
		coarse_reduction *= rate;
	}
	const unsigned int coarse_offset_x = offset_x / coarse_reduction;
	const unsigned int coarse_offset_y = offset_y / coarse_reduction;
	__global const unsigned long *chart = getChart(charts, chartSizes, level, rate, offset_x, offset_y);
	__global const unsigned int *chartSize = getChartSize(chartSizes, level, rate, offset_x, offset_y);
	__global const unsigned long *atlas = getAtlas(atlases, atlasSizes, level);
	__global const unsigned int *atlasSize = getAtlasSize(atlasSizes, level);
	if (coarse_offset_x > atlasSize[0] - chartSize[0]
		|| coarse_offset_y > atlasSize[1] - chartSize[1]) {
		printf("Too bad! (%d, %d) > (%d, %d)\n", coarse_offset_x + chartSize[0], coarse_offset_y + chartSize[1], atlasSize[0], atlasSize[1]);

		return false;
	}

//	printf("Atlas: %dx%d\tChart: %dx%d\tWhere:(%d, %d)\n", atlasSize[0], atlasSize[1], chartSize[0], chartSize[1], offset_x, offset_y);
//	return canBlit(atlas, chart, coarse_offset_x, coarse_offset_y, atlasSize, chartSize, best);
	return canBlit(atlas, chart, coarse_offset_x, coarse_offset_y, atlasSize, chartSize);
}

__kernel void blit(__global const unsigned long *atlases,
				   __global const unsigned long *charts,
				   const int limit_w,
				   const int limit_h,
				   const int levels,
				   const int rate,
				   __global const unsigned int *atlasSizes,
				   __global const unsigned int *chartSizes,
				   __global      unsigned int *best,
				   __global      unsigned char *results)
{
	const unsigned int offset_x = get_global_id(0);
	const unsigned int offset_y = get_global_id(1);

//
	// fast opt-out
	unsigned int best_cpy = *best;
	unsigned int b_x = best_cpy >> 16;
	unsigned int b_y = best_cpy & UINT16_MAX;

	const unsigned int local_id_x = get_local_id(0);
	const unsigned int local_id_y = get_local_id(0);

	const unsigned int work_size_x = get_local_size(0);
	const unsigned int work_size_y = get_local_size(1);

	const unsigned int work_id_x = get_group_id(0);
	const unsigned int work_id_y = get_group_id(1);

	if (work_size_x * work_id_x + local_id_x > b_x && work_size_y * work_id_y + local_id_y > b_y) {
		results[offset_y * (limit_w + 1) + offset_x] = '\0';
	}

	if (offset_x > limit_w || offset_y > limit_h) {
		return;
	}
	if (offset_x + chartSizes[0] > atlasSizes[0] || offset_y + chartSizes[1] > atlasSizes[1]) {
		results[offset_y * (limit_w + 1) + offset_x] = '\0';
		return;
	}

	bool result = false;
	for (int coarse_level = levels - 1; coarse_level >= 0; --coarse_level) {
//		if (!iterateCanBlit(atlases, charts, coarse_level, rate, atlasSizes, chartSizes, offset_x, offset_y, best)) {
		if (!iterateCanBlit(atlases, charts, coarse_level, rate, atlasSizes, chartSizes, offset_x, offset_y)) {
			break;
		}
		if (coarse_level == 0) {
			result = true;
		}
	}

	if (result) {
		unsigned int best_cpy = *best;
		unsigned int b_x = best_cpy >> 16;
		unsigned int b_y = best_cpy & UINT16_MAX;
		if (offset_x < b_x && offset_y < b_y) {
			*best = (offset_x << 16) + offset_y;
		}
	}
	results[offset_y * (limit_w + 1) + offset_x] = result ? '\1' : '\0';
}

__kernel void blitLevel(__global const unsigned long *atlases,
						__global const unsigned long *charts,
						const int w, const int h,
						const int limit_w,
						const int limit_h,
						const int level,
						const int rate,
						__global const unsigned int *atlasSizes,
						__global const unsigned int *chartSizes,
						const short filtered,
						const unsigned int candidates_num,
						__global const unsigned short *filter_x,
						__global const unsigned short *filter_y,
						__global unsigned int *best_metric,
						__global unsigned int *results)
{
	unsigned int offset_x;
	unsigned int offset_y;

	if (filtered) {
		const unsigned int gid = get_global_id(0);
		if (gid >= candidates_num)
			return;
		offset_x = filter_x[gid];
		offset_y = filter_y[gid];
	} else {
		if (level == 0) {
			offset_x = get_global_id(0);
			offset_y = get_global_id(1);
		} else {
			int coarse_reduction = 1;
			for (int i = 0; i < level; ++i) {
				coarse_reduction *= rate;
			}
			const unsigned int wid_x = get_group_id(0);
			const unsigned int wid_y = get_group_id(1);

			unsigned int off_x[3], off_y[3];
			// offset of WorkGroup
			off_x[2] = (wid_x / coarse_reduction) * coarse_reduction * get_local_size(0);
			off_y[2] = (wid_y / coarse_reduction) * coarse_reduction * get_local_size(1);

			// offset within WorkGroup
			off_x[1] = get_local_id(0) * coarse_reduction;
			off_y[1] = get_local_id(1) * coarse_reduction;

			// choose chart template
			off_x[0] = wid_x % coarse_reduction;
			off_y[0] = wid_y % coarse_reduction;

			// make it so every work item has the same chart template
			offset_x = off_x[0] + off_x[1] + off_x[2];
			offset_y = off_y[0] + off_y[1] + off_y[2];
		}
	}

	// Check if point is within limits
	if (offset_x > limit_w || offset_y > limit_h) {
		return;
	}

	// Check if chart fits in atlas here
	if (offset_x + chartSizes[0] > atlasSizes[0] || offset_y + chartSizes[1] > atlasSizes[1]) {
		results[offset_y * (limit_w + 1) + offset_x] = 0;
		return;
	}

	// Check if point is inarguably worse than previously chosen
	unsigned int metric;
	unsigned int cmp_x;
	unsigned int cmp_y;
	if (level == 0) {
		const unsigned int extentX = max(w, offset_x + chartSizes[0]);
		const unsigned int extentY = max(h, offset_y + chartSizes[1]);
		const unsigned int area = extentX * extentY;
		const unsigned int extents = max(extentX, extentY);
		metric = extents * extents + area;

		const unsigned int metric_cmp = best_metric[1];
		// WARNING: RESOLUTION IS LIMITED BY 65535 to make the operation atomic (see 9203458923756982)
		cmp_x = metric_cmp % 0xFFFF;
		cmp_y = metric_cmp / 0xFFFF;
		if (metric > best_metric[0] || (offset_x > cmp_x && offset_y > cmp_y)) {
			results[offset_y * (limit_w + 1) + offset_x] = 0;
			return;
		}
	}

	bool result = iterateCanBlit(atlases, charts, level, rate, atlasSizes, chartSizes, offset_x, offset_y);

	if (level == 0) {
		if (result && metric < best_metric[0]) {
			best_metric[0] = metric;
			const unsigned int metric_cmp = best_metric[1];
			cmp_x = metric_cmp % 0xFFFF;
			cmp_y = metric_cmp / 0xFFFF;
			if (offset_x < cmp_x && offset_y < cmp_y) {
				best_metric[1] = offset_x + (offset_y * 0xFFFF);
//				best_metric[2] = offset_y;
			}
		}
	}
	results[offset_y * (limit_w + 1) + offset_x] = result ? 1 : 0;
}

__kernel void bufferCleanup(__global unsigned int *array,
							const unsigned long n)
{
	const unsigned int gid = get_global_id(0);
	if (gid < n)
		array[gid] = 0;
}

// scan - in-place prefix sum.
// Reduce - pre-calculate every (offset*2)-th element
// offset: must be a power of 2; offset * 4 <= limit
// temp:   every i-th element (where (i + 1) = a * 2^offset  ) stores sum of [(a-1) * 2^offset, a * 2^offset - 1]
__kernel void scanReduce(__global unsigned int *array,
						  const unsigned long limit,
						  const unsigned long offset)
{
	const unsigned int gid = get_global_id(0);
//	const unsigned int limit = limit_w * limit_h;
	unsigned int index_cur = (gid + 1) * offset * 2 - 1;
	if (index_cur >= limit)
		return;

	unsigned int index_prev = index_cur - offset;
//	if (index_prev >= index_cur)
//		printf("SOMETHING'S WRONG!\n");
	array[index_cur] += array[index_prev];
}

// scan - in-place prefix sum.
// DownSweep - calculate every other (offset*2)-th element
// offset: must be a power of 2; offset * 4 <= limit
__kernel void scanDownSweep(__global unsigned int *array,
							  const unsigned long limit,
							  const unsigned long offset)
{
	const unsigned int gid = get_global_id(0);
//	const unsigned int limit = limit_w * limit_h;
	unsigned long index_prev = (gid + 1) * offset * 2 - 1;
	unsigned long index_cur = index_prev + offset;
	if (index_cur >= limit)
		return;

//	if (index_prev >= index_cur)
//		printf("SOMETHING'S WRONG! %u: %lu >= %lu\n  Offset: %lu, limit: %lu\n", gid, index_prev, index_cur, offset, limit);
	array[index_cur] += array[index_prev];
}

__kernel void aggregateResults(__global const unsigned int *array,
							  const int limit_w, const int limit_h,
							  __global unsigned short *results_x,
							  __global unsigned short *results_y)
{
	const unsigned int gid = get_global_id(0);
	if (gid >= (unsigned int)((limit_w + 1) * (limit_h + 1)))
		return;

	int write;
	unsigned int idx = array[gid];

	if (gid == 0) {
		write = (idx != 0) ? 1 : 0;
	}
	else {
		unsigned int idy = array[gid - 1];
		write = (idx > idy) ? 1 : 0;
//		if (idx < idy)
//			printf("SOMETHING'S WRONG! %d: %d < %d\n", gid, idx, idy);
	}

	if (write) {
		--idx;
		results_x[idx] = (unsigned short) (gid % (limit_w + 1));
		results_y[idx] = (unsigned short) (gid / (limit_w + 1));
	}
}