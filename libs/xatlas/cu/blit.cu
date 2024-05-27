#include <libgpu/cuda/cu/opencl_translator.cu>

#include "../cl/blit.cl"
#include "libgpu/work_size.h"

void cuda_blit(const gpu::WorkSize &workSize,
			   const unsigned long *atlases, const unsigned long *charts,
			   const int limit_w,
			   const int limit_h,
			   const int levels,
			   const int rate,
			   const unsigned int *atlasSizes,
			   const unsigned int *chartSizes,
			   unsigned int *best,
			   unsigned char *results,
			   cudaStream_t stream)
{
	blit<<<workSize.cuGridSize(), workSize.cuBlockSize(), 0, stream>>>(atlases, charts,
																	   limit_w, limit_h,
																	   levels,
																	   rate,
																	   atlasSizes,
																	   chartSizes,
																	   best,
																	   results);
	CUDA_CHECK_KERNEL(stream);
}

void cuda_blitLevel(const gpu::WorkSize &workSize,
					const unsigned long *atlases,
					const unsigned long *charts,
					const int w, const int h,
					const int limit_w,
					const int limit_h,
					const int level,
					const int rate,
					const unsigned int *atlasSizes,
					const unsigned int *chartSizes,
					unsigned int *best_metric,
					unsigned int *results,
					cudaStream_t stream)
{
	blitLevel<<<workSize.cuGridSize(), workSize.cuBlockSize(), 0, stream>>>(atlases, charts,
																			w, h,
																			limit_w, limit_h,
																			level,
																			rate,
																			atlasSizes,
																			chartSizes,
																			false,
																			0,
																			nullptr,
																			nullptr,
																			best_metric,
																			results);
}


void cuda_blitFiltered(const gpu::WorkSize &workSize,
					   const unsigned long *atlases,
					   const unsigned long *charts,
					   const int w, const int h,
					   const int limit_w,
					   const int limit_h,
					   const int level,
					   const int rate,
					   const unsigned int *atlasSizes,
					   const unsigned int *chartSizes,
					   const bool filtered,
					   const unsigned int candidates_num,
					   const unsigned short *filter_x,
					   const unsigned short *filter_y,
					   unsigned int *best_metric,
					   unsigned int *results,
					   cudaStream_t stream)
{
	blitLevel<<<workSize.cuGridSize(), workSize.cuBlockSize(), 0, stream>>>(atlases, charts,
																			w, h,
																			limit_w, limit_h,
																			level,
																			rate,
																			atlasSizes,
																			chartSizes,
																			filtered,
																			candidates_num,
																			filter_x,
																			filter_y,
																			best_metric,
																			results);
}

void cuda_bufferCleanup(const gpu::WorkSize &workSize,
					 unsigned int *array,
					 const uint64_t n,
					 cudaStream_t stream)
{
	bufferCleanup<<<workSize.cuGridSize(), workSize.cuBlockSize(), 0, stream>>>(array, n);
}

void cuda_scanReduce(const gpu::WorkSize &workSize,
					 unsigned int *array,
					 const uint64_t limit,
					 const uint64_t offset,
					 cudaStream_t stream)
{
	scanReduce<<<workSize.cuGridSize(), workSize.cuBlockSize(), 0, stream>>>(array, limit, offset);
}

void cuda_scanDownSweep(const gpu::WorkSize &workSize,
						unsigned int *array,
						const uint64_t limit,
						const uint64_t offset,
						cudaStream_t stream)
{
	scanDownSweep<<<workSize.cuGridSize(), workSize.cuBlockSize(), 0, stream>>>(array, limit, offset);
}


void cuda_aggregateResults(const gpu::WorkSize &workSize,
						   const unsigned int *array,
						   const int limit_w, const int limit_h,
						   unsigned short *results_x,
						   unsigned short *results_y,
						   cudaStream_t stream)
{
	aggregateResults<<<workSize.cuGridSize(), workSize.cuBlockSize(), 0, stream>>>(array, limit_w, limit_h, results_x, results_y);
}