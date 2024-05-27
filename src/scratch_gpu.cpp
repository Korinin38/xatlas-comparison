
#include <climits>
#include <cstdint>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <vector>

#include "parallel_tools.h"
#include "cl/blit_cl.h"

#include <libgpu/context.h>
#include <libgpu/shared_device_buffer.h>
#include <libgpu/device.h>
#include <libutils/misc.h>

#ifndef XA_ASSERT
#define XA_ASSERT(exp) if (!(exp)) { XA_PRINT_WARNING("\rASSERT: %s %s %d\n", XA_XSTR(exp), __FILE__, __LINE__); }
#endif


#ifndef XA_DEBUG_ASSERT
#define XA_DEBUG_ASSERT(exp) assert(exp)
#endif

#define XA_UNUSED(a) ((void)(a))

#define XA_ALLOC(tag, type) (type *)internal::Realloc(nullptr, sizeof(type), tag, __FILE__, __LINE__)
#define XA_ALLOC_ARRAY(tag, type, num) (type *)internal::Realloc(nullptr, sizeof(type) * (num), tag, __FILE__, __LINE__)
#define XA_REALLOC(tag, ptr, type, num) (type *)internal::Realloc(ptr, sizeof(type) * (num), tag, __FILE__, __LINE__)
#define XA_REALLOC_SIZE(tag, ptr, size) (uint8_t *)internal::Realloc(ptr, size, tag, __FILE__, __LINE__)
#define XA_FREE(ptr) internal::Realloc(ptr, 0, internal::MemTag::Default, __FILE__, __LINE__)
#define XA_NEW(tag, type) new (XA_ALLOC(tag, type)) type()
#define XA_NEW_ARGS(tag, type, ...) new (XA_ALLOC(tag, type)) type(__VA_ARGS__)

#ifdef _MSC_VER
#define XA_INLINE __forceinline
#else
#define XA_INLINE inline
#endif


namespace internal {
	typedef void *(*ReallocFunc)(void *, size_t);

	typedef void (*FreeFunc)(void *);

	static ReallocFunc s_realloc = realloc;
	static FreeFunc s_free = free;

	static void *Realloc(void *ptr, size_t size, int /*tag*/, const char * /*file*/, int /*line*/) {
		if (size == 0 && !ptr)
			return nullptr;
		if (size == 0 && s_free) {
			s_free(ptr);
			return nullptr;
		}
#if XA_PROFILE_ALLOC
		XA_PROFILE_START(alloc)
#endif
		void *mem = s_realloc(ptr, size);
#if XA_PROFILE_ALLOC
		XA_PROFILE_END(alloc)
#endif
		XA_DEBUG_ASSERT(size <= 0 || (size > 0 && mem));
		return mem;
	}

	struct MemTag {
		enum {
			Default,
			BitImage,
			BVH,
			Matrix,
			Mesh,
			MeshBoundaries,
			MeshColocals,
			MeshEdgeMap,
			MeshIndices,
			MeshNormals,
			MeshPositions,
			MeshTexcoords,
			OpenNL,
			SegmentAtlasChartCandidates,
			SegmentAtlasChartFaces,
			SegmentAtlasMeshData,
			SegmentAtlasPlanarRegions,
			Count
		};
	};

struct ArrayBase
{
	ArrayBase(uint32_t _elementSize, int memTag = MemTag::Default) : buffer(nullptr), elementSize(_elementSize), size(0), capacity(0)
	{
#if XA_DEBUG_HEAP
		this->memTag = memTag;
#else
		XA_UNUSED(memTag);
#endif
	}

	~ArrayBase()
	{
		XA_FREE(buffer);
	}

	XA_INLINE void clear()
	{
		size = 0;
	}

	void copyFrom(const uint8_t *data, uint32_t length)
	{
		XA_DEBUG_ASSERT(data);
		XA_DEBUG_ASSERT(length > 0);
		resize(length, true);
		if (buffer && data && length > 0)
			memcpy(buffer, data, length * elementSize);
	}

	void copyTo(ArrayBase &other) const
	{
		XA_DEBUG_ASSERT(elementSize == other.elementSize);
		XA_DEBUG_ASSERT(size > 0);
		other.resize(size, true);
		if (other.buffer && buffer && size > 0)
			memcpy(other.buffer, buffer, size * elementSize);
	}

	void destroy()
	{
		size = 0;
		XA_FREE(buffer);
		buffer = nullptr;
		capacity = 0;
		size = 0;
	}

	// Insert the given element at the given index shifting all the elements up.
	void insertAt(uint32_t index, const uint8_t *value)
	{
		XA_DEBUG_ASSERT(index >= 0 && index <= size);
		XA_DEBUG_ASSERT(value);
		resize(size + 1, false);
		XA_DEBUG_ASSERT(buffer);
		if (buffer && index < size - 1)
			memmove(buffer + elementSize * (index + 1), buffer + elementSize * index, elementSize * (size - 1 - index));
		if (buffer && value)
			memcpy(&buffer[index * elementSize], value, elementSize);
	}

	void moveTo(ArrayBase &other)
	{
		XA_DEBUG_ASSERT(elementSize == other.elementSize);
		other.destroy();
		other.buffer = buffer;
		other.elementSize = elementSize;
		other.size = size;
		other.capacity = capacity;
#if XA_DEBUG_HEAP
		other.memTag = memTag;
#endif
		buffer = nullptr;
		elementSize = size = capacity = 0;
	}

	void pop_back()
	{
		XA_DEBUG_ASSERT(size > 0);
		resize(size - 1, false);
	}

	void push_back(const uint8_t *value)
	{
		XA_DEBUG_ASSERT(value < buffer || value >= buffer + size);
		XA_DEBUG_ASSERT(value);
		resize(size + 1, false);
		XA_DEBUG_ASSERT(buffer);
		if (buffer && value)
			memcpy(&buffer[(size - 1) * elementSize], value, elementSize);
	}

	void push_back(const ArrayBase &other)
	{
		XA_DEBUG_ASSERT(elementSize == other.elementSize);
		if (other.size > 0) {
			const uint32_t oldSize = size;
			resize(size + other.size, false);
			XA_DEBUG_ASSERT(buffer);
			if (buffer)
				memcpy(buffer + oldSize * elementSize, other.buffer, other.size * other.elementSize);
		}
	}

	// Remove the element at the given index. This is an expensive operation!
	void removeAt(uint32_t index)
	{
		XA_DEBUG_ASSERT(index >= 0 && index < size);
		XA_DEBUG_ASSERT(buffer);
		if (buffer) {
			if (size > 1)
				memmove(buffer + elementSize * index, buffer + elementSize * (index + 1), elementSize * (size - 1 - index));
			if (size > 0)
				size--;
		}
	}

	// Element at index is swapped with the last element, then the array length is decremented.
	void removeAtFast(uint32_t index)
	{
		XA_DEBUG_ASSERT(index >= 0 && index < size);
		XA_DEBUG_ASSERT(buffer);
		if (buffer) {
			if (size > 1 && index != size - 1)
				memcpy(buffer + elementSize * index, buffer + elementSize * (size - 1), elementSize);
			if (size > 0)
				size--;
		}
	}

	void reserve(uint32_t desiredSize)
	{
		if (desiredSize > capacity)
			setArrayCapacity(desiredSize);
	}

	void resize(uint32_t newSize, bool exact)
	{
		size = newSize;
		if (size > capacity) {
			// First allocation is always exact. Otherwise, following allocations grow array to 150% of desired size.
			uint32_t newBufferSize;
			if (capacity == 0 || exact)
				newBufferSize = size;
			else
				newBufferSize = size + (size >> 2);
			setArrayCapacity(newBufferSize);
		}
	}

	void setArrayCapacity(uint32_t newCapacity)
	{
		XA_DEBUG_ASSERT(newCapacity >= size);
		if (newCapacity == 0) {
			// free the buffer.
			if (buffer != nullptr) {
				XA_FREE(buffer);
				buffer = nullptr;
			}
		} else {
			// realloc the buffer
#if XA_DEBUG_HEAP
			buffer = XA_REALLOC_SIZE(memTag, buffer, newCapacity * elementSize);
#else
			buffer = XA_REALLOC_SIZE(MemTag::Default, buffer, newCapacity * elementSize);
#endif
		}
		capacity = newCapacity;
	}

#if XA_DEBUG_HEAP
	void setMemTag(int _memTag)
	{
		this->memTag = _memTag;
	}
#endif

	uint8_t *buffer;
	uint32_t elementSize;
	uint32_t size;
	uint32_t capacity;
#if XA_DEBUG_HEAP
	int memTag;
#endif
};

template <typename T>
static T min(const T &a, const T &b)
{
	return a < b ? a : b;
}

template <typename T>
static T max(const T &a, const T &b)
{
	return a > b ? a : b;
}

template<typename T>
class Array
{
public:
	Array(int memTag = MemTag::Default) : m_base(sizeof(T), memTag) {}
	Array(const Array&) = delete;
	Array &operator=(const Array &) = delete;

	XA_INLINE const T &operator[](uint32_t index) const
	{
		XA_DEBUG_ASSERT(index < m_base.size);
		XA_DEBUG_ASSERT(m_base.buffer);
		return ((const T *)m_base.buffer)[index];
	}

	XA_INLINE T &operator[](uint32_t index)
	{
		XA_DEBUG_ASSERT(index < m_base.size);
		XA_DEBUG_ASSERT(m_base.buffer);
		return ((T *)m_base.buffer)[index];
	}

	XA_INLINE const T &back() const
	{
		XA_DEBUG_ASSERT(!isEmpty());
		return ((const T *)m_base.buffer)[m_base.size - 1];
	}

	XA_INLINE T *begin() { return (T *)m_base.buffer; }
	XA_INLINE void clear() { m_base.clear(); }

	bool contains(const T &value) const
	{
		for (uint32_t i = 0; i < m_base.size; i++) {
			if (((const T *)m_base.buffer)[i] == value)
				return true;
		}
		return false;
	}

	void copyFrom(const T *data, uint32_t length) { m_base.copyFrom((const uint8_t *)data, length); }
	void copyTo(Array &other) const { m_base.copyTo(other.m_base); }
	XA_INLINE const T *data() const { return (const T *)m_base.buffer; }
	XA_INLINE T *data() { return (T *)m_base.buffer; }
	void destroy() { m_base.destroy(); }
	XA_INLINE T *end() { return (T *)m_base.buffer + m_base.size; }
	XA_INLINE bool isEmpty() const { return m_base.size == 0; }
	void insertAt(uint32_t index, const T &value) { m_base.insertAt(index, (const uint8_t *)&value); }
	void moveTo(Array &other) { m_base.moveTo(other.m_base); }
	void push_back(const T &value) { m_base.push_back((const uint8_t *)&value); }
	void push_back(const Array &other) { m_base.push_back(other.m_base); }
	void pop_back() { m_base.pop_back(); }
	void removeAt(uint32_t index) { m_base.removeAt(index); }
	void removeAtFast(uint32_t index) { m_base.removeAtFast(index); }
	void reserve(uint32_t desiredSize) { m_base.reserve(desiredSize); }
	void resize(uint32_t newSize) { m_base.resize(newSize, true); }

	void runCtors()
	{
		for (uint32_t i = 0; i < m_base.size; i++)
			new (&((T *)m_base.buffer)[i]) T;
	}

	void runDtors()
	{
		for (uint32_t i = 0; i < m_base.size; i++)
			((T *)m_base.buffer)[i].~T();
	}

	void fill(const T &value)
	{
		auto buffer = (T *)m_base.buffer;
		for (uint32_t i = 0; i < m_base.size; i++)
			buffer[i] = value;
	}

	void fillBytes(uint8_t value)
	{
		if (m_base.buffer && m_base.size > 0)
			memset(m_base.buffer, (int)value, m_base.size * m_base.elementSize);
	}

#if XA_DEBUG_HEAP
	void setMemTag(int memTag) { m_base.setMemTag(memTag); }
#endif

	XA_INLINE uint32_t size() const { return m_base.size; }

	XA_INLINE void zeroOutMemory()
	{
		if (m_base.buffer && m_base.size > 0)
			memset(m_base.buffer, 0, m_base.elementSize * m_base.size);
	}

private:
	ArrayBase m_base;
};

class BitImage
{
public:
	BitImage() : m_width(0), m_height(0), m_rowStride(0), m_data(MemTag::BitImage) {}

	BitImage(uint32_t w, uint32_t h) : m_width(w), m_height(h), m_data(MemTag::BitImage)
	{
		m_rowStride = (m_width + 63) >> 6;
		m_data.resize(m_rowStride * m_height);
		m_data.zeroOutMemory();
	}

	BitImage(const BitImage &other) = delete;
	BitImage &operator=(const BitImage &other) = delete;
	uint32_t width() const { return m_width; }
	uint32_t height() const { return m_height; }
	uint32_t stride() const { return m_rowStride; }

	const uint64_t *data() const { return m_data.data(); }

	void copyTo(BitImage &other) const
	{
		other.m_width = m_width;
		other.m_height = m_height;
		other.m_rowStride = m_rowStride;
		m_data.copyTo(other.m_data);
	}

	void resize(uint32_t w, uint32_t h, bool discard)
	{
		const uint32_t rowStride = (w + 63) >> 6;
		if (discard) {
			m_data.resize(rowStride * h);
			m_data.zeroOutMemory();
		} else {
			Array<uint64_t> tmp;
			tmp.resize(rowStride * h);
			memset(tmp.data(), 0, tmp.size() * sizeof(uint64_t));
			// If only height has changed, can copy all rows at once.
			if (rowStride == m_rowStride) {
				memcpy(tmp.data(), m_data.data(), m_rowStride * min(m_height, h) * sizeof(uint64_t));
			} else if (m_width > 0 && m_height > 0) {
				const uint32_t height = min(m_height, h);
				for (uint32_t i = 0; i < height; i++)
					memcpy(&tmp[i * rowStride], &m_data[i * m_rowStride], min(rowStride, m_rowStride) * sizeof(uint64_t));
			}
			tmp.moveTo(m_data);
		}
		m_width = w;
		m_height = h;
		m_rowStride = rowStride;
	}

	bool get(uint32_t x, uint32_t y) const
	{
		XA_DEBUG_ASSERT(x < m_width && y < m_height);
		const uint32_t index = (x >> 6) + y * m_rowStride;
		return (m_data[index] & (UINT64_C(1) << (uint64_t(x) & UINT64_C(63)))) != 0;
	}

	void set(uint32_t x, uint32_t y)
	{
		XA_DEBUG_ASSERT(x < m_width && y < m_height);
		const uint32_t index = (x >> 6) + y * m_rowStride;
		m_data[index] |= UINT64_C(1) << (uint64_t(x) & UINT64_C(63));
		XA_DEBUG_ASSERT(get(x, y));
	}

	void zeroOutMemory()
	{
		m_data.zeroOutMemory();
	}

	bool canBlit(const BitImage &image, uint32_t offsetX, uint32_t offsetY) const
	{
		for (uint32_t y = 0; y < image.m_height; y++) {
			const uint32_t thisY = y + offsetY;
			if (thisY >= m_height)
				continue;
			uint32_t x = 0;
			for (;;) {
				const uint32_t thisX = x + offsetX;
				if (thisX >= m_width)
					break;
				const uint32_t thisBlockShift = thisX % 64;
				const uint64_t thisBlock = m_data[(thisX >> 6) + thisY * m_rowStride] >> thisBlockShift;
				const uint32_t blockShift = x % 64;
				const uint64_t block = image.m_data[(x >> 6) + y * image.m_rowStride] >> blockShift;
				if ((thisBlock & block) != 0)
					return false;
				x += 64 - max(thisBlockShift, blockShift);
				if (x >= image.m_width)
					break;
			}
		}
		return true;
	}

	void dilate(uint32_t padding)
	{
		BitImage tmp(m_width, m_height);
		for (uint32_t p = 0; p < padding; p++) {
			tmp.zeroOutMemory();
			for (uint32_t y = 0; y < m_height; y++) {
				for (uint32_t x = 0; x < m_width; x++) {
					bool b = get(x, y);
					if (!b) {
						if (x > 0) {
							b |= get(x - 1, y);
							if (y > 0) b |= get(x - 1, y - 1);
							if (y < m_height - 1) b |= get(x - 1, y + 1);
						}
						if (y > 0) b |= get(x, y - 1);
						if (y < m_height - 1) b |= get(x, y + 1);
						if (x < m_width - 1) {
							b |= get(x + 1, y);
							if (y > 0) b |= get(x + 1, y - 1);
							if (y < m_height - 1) b |= get(x + 1, y + 1);
						}
					}
					if (b)
						tmp.set(x, y);
				}
			}
			tmp.m_data.copyTo(m_data);
		}
	}

	// This BitImage would be reduced into *image by rules:
	//   Image is divided into blocks of size rate x rate
	//   (depending on pessimistic=false/true, respectively) -
	//   If block has any/all bits set to 1:
	//     Set respective bit of reduced image to 1.
	void reduceTo(BitImage *image, int rate, int offset_x, int offset_y, bool pessimistic) const
	{
		XA_DEBUG_ASSERT(image != nullptr);
		XA_DEBUG_ASSERT(image != this);
//		XA_ASSERT(offset_x >= 0);
//		XA_ASSERT(offset_y >= 0);
		offset_x %= rate;
		offset_y %= rate;

		if (pessimistic) {
			reduceToPessimistic(image, rate, offset_x, offset_y);
		} else {
			reduceToOptimistic(image, rate, offset_x, offset_y);
		}
	}

	void rotateTo(BitImage *image1, BitImage *image2, BitImage *image3) const {
		XA_DEBUG_ASSERT(image1 != this);
		XA_DEBUG_ASSERT(image2 != this);
		XA_DEBUG_ASSERT(image3 != this);

		BitImage *images[4];
		images[1] = image1;
		images[2] = image2;
		images[3] = image3;

		for (int r = 1; r < 4; ++r) {
			if (r % 2 == 0)
				images[r]->resize(m_width, m_height, true);
			else
				images[r]->resize(m_height, m_width, true);
		}

		for (int y = 0; y < m_height; ++y) {
			for (int x = 0; x < m_width; ++x) {
				if (get(x, y)) {
					if (images[1] != nullptr)
						images[1]->set(m_height - y - 1, x);
					if (images[2] != nullptr)
						images[2]->set(m_width - x - 1, m_height - y - 1);
					if (images[3] != nullptr)
						images[3]->set(y, m_width - x - 1);
				}
			}
		}
	}

private:
	void reduceToPessimistic(BitImage *image, int rate, int offset_x, int offset_y) const
	{
		image->resize((m_width + offset_x + rate - 1) / rate, (m_height + offset_y + rate - 1) / rate, true);

#if XA_OMP_PARALLEL
#pragma omp parallel for collapse(2)
#endif // XA_OMP_PARALLEL
		for (int y = 0; y < image->height(); ++y) {
			for (int x = 0; x < image->width(); ++x) {
				if ((x + 1) * rate - offset_x > (int)m_width || (y + 1) * rate - offset_y > (int)m_height)
					continue;

				bool all = true;
				// traversal within reduction square
				for (int yy = y * rate - offset_y; yy < (y + 1) * rate - offset_y && yy < (int)m_height; ++yy) {
					for (int xx = x * rate - offset_x; xx < (x + 1) * rate - offset_x && xx < (int)m_width; ++xx) {
						if (yy < 0 || xx < 0 || !get(xx, yy)) {
							all = false;
							break;
						}
					}
					if (!all)
						break;
				}

				if (all)
					image->set(x, y);
			}
		}
	}

	void reduceToOptimistic(BitImage *image, int rate, int offset_x, int offset_y) const
	{
		image->resize((m_width + offset_x + rate - 1) / rate, (m_height + offset_y + rate - 1) / rate, true);

#if REDUCE_OPTIMISTIC_SPEEDUP
		BitImage rect;
		rect.resize(rate, rate, true);
		for (int yy = 0; yy < rate; ++yy) {
			for (int xx = 0; xx < rate; ++xx) {
				rect.set(xx, yy);
			}
		}
#endif // REDUCE_OPTIMISTIC_SPEEDUP

#if XA_OMP_PARALLEL
#pragma omp parallel for collapse(2)
#endif // XA_OMP_PARALLEL
		for (int y = 0; y < image->height(); ++y) {
			for (int x = 0; x < image->width(); ++x) {
				// fast check using existing code; does not work near edges
#if REDUCE_OPTIMISTIC_SPEEDUP
				if (x * rate - offset_x >= 0 && (x + 1) * rate - offset_x <= (int)m_width
				&& y * rate - offset_y >= 0 && (y + 1) * rate - offset_y <= (int)m_height) {
					if (!canBlit(rect, x * rate - offset_x, y * rate - offset_y))
						image->set(x, y);
					continue;
				}
#endif // REDUCE_OPTIMISTIC_SPEEDUP
				bool any = false;
				// traversal within reduction square
				for (int yy = y * rate - offset_y; yy < (y + 1) * rate - offset_y && yy < (int)m_height; ++yy) {
					for (int xx = x * rate - offset_x; xx < (x + 1) * rate - offset_x && xx < (int)m_width; ++xx) {
						if (yy >= 0 && xx >= 0 && get(xx, yy)) {
							any = true;
							break;
						}
					}
					if (any)
						break;
				}

				if (any)
					image->set(x, y);
			}
		}
	}

	uint32_t m_width;
	uint32_t m_height;
	uint32_t m_rowStride; // In uint64_t's
	Array<uint64_t> m_data;
};

struct Vector2i
{
	Vector2i() {}
	Vector2i(int32_t _x, int32_t _y) : x(_x), y(_y) {}

	int32_t x, y;
};

class CoarsePyramid {
	public:
		CoarsePyramid(const BitImage *image, const BitImage *imageTransposed, int levels, int rate, bool rotate = false) : m_levels(levels), m_rate(rate), m_rotated(rotate) {
			XA_DEBUG_ASSERT(image != nullptr);
			m_orientations = 1;
			if (imageTransposed != nullptr) {
				m_transposed = true;
				m_orientations *= 2;
			}
			if (m_rotated)
				m_orientations *= 4;

			if (m_levels == 1 || m_rate == 1) {
				m_levels = 1;
				m_rate = 1;
			}

// making hard limit on levels; charts that are small enough would not be reduced
#define XA_COARSE_MAX_REDUCTION_SIZE 64
			int extents = max(image->width(), image->height());
			levels = 1;
			while (extents > XA_COARSE_MAX_REDUCTION_SIZE && levels < m_levels) {
				extents /= m_rate;
				++levels;
			}
			m_levels = levels;

			uint32_t num_images;
			if (m_levels == 1) {
				m_rate = 1;
				num_images = m_orientations;
			} else {
				uint32_t rate_squared = rate * rate;
				uint64_t rate_pow_levels = 1;

				for (int i = 0; i < m_levels; ++i) {
					rate_pow_levels *= rate_squared;
				}
				num_images = (rate_pow_levels - 1) / (rate_squared - 1) * m_orientations;
			}
			m_data.resize(num_images);
			m_data.runCtors();

			m_data[0].resize(image->width(), image->height(), true);
			image->copyTo(m_data[0]);
			if (m_transposed) {
				int r = m_rotated ? 4 : 1;
				m_data[r].resize(image->height(), image->width(), true);
				imageTransposed->copyTo(m_data[r]);
			}

			// if both transposed and rotated:
			// the order of transposes and rotations is:
			// 0 1 2 3 rotations, 4 5 6 7 transposed rotations
			if (m_rotated) {
				m_data[0].rotateTo(&m_data[1], &m_data[2], &m_data[3]);
				if (m_transposed) {
					m_data[4].rotateTo(&m_data[5], &m_data[6], &m_data[7]);
				}
			}

			for (int level = 1; level < m_levels; ++level) {
				for (int r = 0; r < m_orientations; ++r) {
					// do something
					int rate_cur = 1;
					for (int i = 0; i < level; ++i)
						rate_cur *= m_rate;
#if XA_OMP_PARALLEL
#pragma omp parallel for collapse(2)
#endif // XA_OMP_PARALLEL
					for (int offset_y = 0; offset_y < rate_cur; offset_y++) {
						for (int offset_x = 0; offset_x < rate_cur; offset_x++) {
							BitImage &parent = get(level - 1, offset_x, offset_y, r);
							BitImage &child = get(level, offset_x, offset_y, r);
							parent.reduceTo(&child, m_rate, offset_x * m_rate / rate_cur, offset_y * m_rate / rate_cur, false);
						}
					}
				}
			}
		}

		~CoarsePyramid() {
			m_data.runDtors();
		}

		const BitImage& get(int level, int offset_x = 0, int offset_y = 0, int orientation = 0) const {
			XA_DEBUG_ASSERT(level < m_levels);
			XA_DEBUG_ASSERT(orientation < 8);
			XA_DEBUG_ASSERT(orientation < 4 || m_transposed);
			XA_DEBUG_ASSERT(m_rotated || (m_transposed && orientation < 2) || orientation == 0);

			// rate on current level
			int cur_level_rate = 1;
			for (int i = 0; i < level; ++i)
				cur_level_rate *= m_rate;
			// number of images on all previous layers
			const int num_offset = m_rate == 1 ? 0 : (cur_level_rate * cur_level_rate - 1) / (m_rate * m_rate - 1) * m_orientations;

			offset_x %= cur_level_rate;
			offset_y %= cur_level_rate;

			return m_data[num_offset + orientation * (cur_level_rate * cur_level_rate) + offset_y * cur_level_rate + offset_x];
		}

		BitImage& get(int level, int offset_x = 0, int offset_y = 0, int orientation = 0) {
			XA_DEBUG_ASSERT(level < m_levels);
			XA_DEBUG_ASSERT(orientation < 8);
			XA_DEBUG_ASSERT(orientation < 4 || m_transposed);
			XA_DEBUG_ASSERT(m_rotated || (m_transposed && orientation < 2) || orientation == 0);

			// rate on current level
			int cur_level_rate = 1;
			for (int i = 0; i < level; ++i)
				cur_level_rate *= m_rate;
			// number of images on all previous layers
			const int num_offset = m_rate == 1 ? 0 : (cur_level_rate * cur_level_rate - 1) / (m_rate * m_rate - 1) * m_orientations;

			offset_x %= cur_level_rate;
			offset_y %= cur_level_rate;

			return m_data[num_offset + orientation * (cur_level_rate * cur_level_rate) + offset_y * cur_level_rate + offset_x];
		}

		XA_INLINE const BitImage& getTransposed(int level, int offset_x = 0, int offset_y = 0, int rotation = 0) const {
			if (m_rotated)
				return get(level, offset_x, offset_y, rotation + 4);
			else {
				XA_DEBUG_ASSERT(rotation == 0);
				return get(level, offset_x, offset_y, 1);
			}
		}

		XA_INLINE BitImage& getTransposed(int level, int offset_x = 0, int offset_y = 0, int rotation = 0) {
			if (m_rotated)
				return get(level, offset_x, offset_y, rotation + 4);
			else {
				XA_DEBUG_ASSERT(rotation == 0);
				return get(level, offset_x, offset_y, 1);
			}
		}

		XA_INLINE const BitImage& getRotation(int level, int rotation, int offset_x = 0, int offset_y = 0, bool transposed = false) const {
			return get(level, offset_x, offset_y, transposed ? rotation + 4 : rotation);
		}

		XA_INLINE BitImage& getRotation(int level, int rotation, int offset_x = 0, int offset_y = 0, bool transposed = false)  {
			return get(level, offset_x, offset_y, transposed ? rotation + 4 : rotation);
		}

		int levels() const {
			return m_levels;
		}

		int orientations() const {
			return m_orientations;
		}

	private:
		Array<BitImage> m_data;
		int m_rate;
		int m_levels;
		bool m_transposed = false;
		bool m_rotated = false;
		int m_orientations = 1;
	};

}

gpu::Device chooseGPUDevice(int idx)
{
	std::vector <gpu::Device> devices = gpu::enumDevices();
	unsigned int device_index = std::numeric_limits<unsigned int>::max();

	if (devices.size() == 0) {
		throw std::runtime_error("No OpenCL devices found!");
	} else {
		std::cout << "OpenCL devices:" << std::endl;
		for (int i = 0; i < devices.size(); ++i) {
			std::cout << "  Device #" << i << ": ";
//			gpu::printDeviceInfo(devices[i]);
		}
		if (devices.size() == 1) {
			device_index = 0;
		} else {
			device_index = idx;
			if (device_index >= devices.size()) {
				std::cerr << "<OpenCLDeviceIndex> should be from 0 to " << (devices.size() - 1) << " (inclusive)! But " << idx << " provided!" << std::endl;
				throw std::runtime_error("Illegal arguments!");
			}
		}
		std::cout << "Using device #" << device_index << ": ";
//		gpu::printDeviceInfo(devices[device_index]);
	}
	return devices[device_index];
}

using namespace internal;
bool findChartLocation_bruteForce_gpu(const Vector2i &startPosition, const Array<BitImage *> *atlasBitImages, const CoarsePyramid &chartBitImages, const int w, const int h, int *best_x, int *best_y, int *best_w, int *best_h, int *best_ori, uint32_t maxResolution)
{
	gpu::Device device = chooseGPUDevice(0);
	unsigned int best_metric = INT_MAX;

	gpu::Context context;
	context.init(device.device_id_opencl);
	context.activate();

	ocl::Kernel kernel(blit_kernel, blit_kernel_length, "blit");
	bool printLog = false;
	kernel.compile(printLog);

	gpu::gpu_mem_64u atlas_gpu;
	gpu::gpu_mem_64u chart_gpu;

	const BitImage *chart = &chartBitImages.get(0, 0, 0, 0);
	int cw = chartBitImages.get(0, 0, 0, 0).width();
	int ch = chartBitImages.get(0, 0, 0, 0).height();

	Vector2i endPosition;
	endPosition.x = w + 1;
	endPosition.y = h + 1;
	if (maxResolution > 0) {
		// with some leeway
		endPosition.x = min(endPosition.x,
							(int32_t) (*atlasBitImages)[0]->width() - cw);
		endPosition.y = min(endPosition.y,
							(int32_t) (*atlasBitImages)[0]->height() - ch);
		// without leeway
		endPosition.x = min(endPosition.x, (int) maxResolution - cw);
		endPosition.y = min(endPosition.y, (int) maxResolution - ch);
	}
	if (startPosition.x > endPosition.x
		|| startPosition.y > endPosition.y)
		return false;

	BitImage *atlas = (*atlasBitImages)[0];

	atlas_gpu.resizeN(atlas->stride() * atlas->height());
	atlas_gpu.writeN(atlas->data(), atlas->stride() * atlas->height());
	chart_gpu.resizeN(chart->stride() * chart->height());
	chart_gpu.writeN(chart->data(), chart->stride() * chart->height());

	gpu::gpu_mem_8u output_gpu;
	output_gpu.resizeN((w + 1) * (h + 1));


	unsigned int workGroupSize = 128;
	kernel.exec(gpu::WorkSize(workGroupSize, workGroupSize, endPosition.x + 1, endPosition.y + 1),
				atlas_gpu,
				chart_gpu,
				w, h,
				cw, ch,
				output_gpu);

	std::vector<unsigned char> results((w + 1) * (h + 1), 0);
	output_gpu.readN(results.data(), (w + 1) * (h + 1));

	// d_infty
	auto sameMetricWorse = [](int x, int y, int ori, int comp_x, int comp_y, int comp_ori) {
		return (max(x, y) > max(comp_x, comp_y)
				|| max(x, y) == max(comp_x, comp_y)
				   && (y > comp_y || y == comp_y
									 && (x > comp_x || x == comp_x && ori > comp_ori)));
	};
	for (int y = 0; y <= endPosition.y; ++y) {
		for (int x = 0; x <= endPosition.x; ++x) {
			if (!results[y * (endPosition.x + 1) + x]) continue;

			const unsigned int extentX = max(w, x + cw);
			const unsigned int extentY = max(h, y + ch);
			const unsigned int area = extentX * extentY;
			const unsigned int extents = max(extentX, extentY);
			const unsigned int metric = extents * extents + area;

			if (metric > best_metric)
				continue;
			if (metric == best_metric &&
				sameMetricWorse(x, y, 0, *best_x, *best_y, 0)) // comparison with *best_x and *best_y
				continue;

			best_metric = metric;
			*best_x = x;
			*best_y = y;
		}
	}

	*best_w = cw;
	*best_h = ch;

	return best_metric != INT_MAX;
}
