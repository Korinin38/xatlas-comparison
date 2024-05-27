//
// Created by agisoft on 13.07.23.
//

#ifndef XATLAS_UVATLAS_COMPARISON_TEST_COMPRESSOR_H
#define XATLAS_UVATLAS_COMPARISON_TEST_COMPRESSOR_H

#include <cstdint>
#include <iostream>
#include <cassert>
#include <cstdlib>
#include <stdexcept>
#include <cstring>

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

    static void *Realloc(void *ptr, size_t size, int /*tag*/, const char * /*file*/, int /*line*/)
    {
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
    struct MemTag
    {
        enum
        {
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

    void shrink(uint32_t padding)
    {
        BitImage tmp(m_width, m_height);
        for (uint32_t p = 0; p < padding; p++) {
            tmp.zeroOutMemory();
            for (uint32_t y = 0; y < m_height; y++) {
                for (uint32_t x = 0; x < m_width; x++) {
                    bool b = get(x, y);
                    if (!b)
                        continue;
                    int neighbors = 0;
                    int non_empty_neighbors = 0;
                    if (b) {
                        if (x > 0) {
                            neighbors++;
                            non_empty_neighbors += get(x - 1, y);
                            if (y > 0) {
                                neighbors++;
                                non_empty_neighbors += get(x - 1, y - 1);
                            }
                            if (y < m_height - 1) {
                                neighbors++;
                                non_empty_neighbors += get(x - 1, y + 1);
                            }
                        }
                        if (y > 0) {
                            neighbors++;
                            non_empty_neighbors += get(x, y - 1);
                        }
                        if (y < m_height - 1) {
                            neighbors++;
                            non_empty_neighbors += get(x, y + 1);
                        }
                        if (x < m_width - 1) {
                            neighbors++;
                            non_empty_neighbors += get(x + 1, y);
                            if (y > 0) {
                                neighbors++;
                                non_empty_neighbors += get(x + 1, y - 1);
                            }
                            if (y < m_height - 1) {
                                neighbors++;
                                non_empty_neighbors += get(x + 1, y + 1);
                            }
                        }
                    }
                    if (non_empty_neighbors == neighbors && neighbors != 0)
                        tmp.set(x, y);
//					if (non_empty_neighbors == 0)
//						tmp.set(x, y);
                }
            }
            tmp.m_data.copyTo(m_data);
        }
    }

private:
    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_rowStride; // In uint64_t's
    Array<uint64_t> m_data;
};

/** Return the next power of two.
* @see http://graphics.stanford.edu/~seander/bithacks.html
* @warning Behaviour for 0 is undefined.
* @note isPowerOfTwo(x) == true -> nextPowerOfTwo(x) == x
* @note nextPowerOfTwo(x) = 2 << log2(x-1)
*/
static uint32_t nextPowerOfTwo(uint32_t x)
{
    XA_DEBUG_ASSERT( x != 0 );
    // On modern CPUs this is supposed to be as fast as using the bsr instruction.
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return x + 1;
}


// Wraps BitImage, adding a list of exact coverage of that image with rectangles, in order of area
class BitImageOpt {
public:
    // let's try simplest, slow-to-construct QuadTree
    BitImageOpt(const BitImage &image) : m_width(image.width()), m_height(image.height()) {
        createRect_recursive(image, {0, 0, image.width() - 1, image.height() - 1});
    }
    struct Rectangle {
        uint32_t start_x;
        uint32_t start_y;
        uint32_t end_x;
        uint32_t end_y;
    };

    void decompressInto(BitImage *image) const {
        XA_DEBUG_ASSERT(image != nullptr);
        image->resize(m_width, m_height, true);
        for (int i = 0; i < m_data.size(); ++i) {
            const Rectangle &rect = m_data[i];
            for (int y = rect.start_y; y <= rect.end_y; ++y) {
                for (int x = rect.start_x; x <= rect.end_x; ++x) {
                    image->set(x, y);
                }
            }
        }
    }

    Array<Rectangle> m_data;
private:
    void createRect_recursive(const BitImage &image, const Rectangle bounds) {
        if (bounds.start_y > bounds.end_y || bounds.start_x > bounds.end_x)
            return;
        if (bounds.start_y == bounds.end_y && bounds.start_x == bounds.end_x) {
            if (image.get(bounds.start_x, bounds.start_y))
                m_data.push_back(bounds);
            return;
        }
        bool has_holes = false;
        for (int y = bounds.start_y; y <= bounds.end_y; ++y) {
            for (int x = bounds.start_x; x <= bounds.end_x; ++x) {
                if (!image.get(x, y)) {
                    has_holes = true;
                    break;
                }
            }
        }
        if (!has_holes) {
            m_data.push_back(bounds);
        } else {
            uint32_t mid_x = (bounds.start_x + bounds.end_x) / 2;
            uint32_t mid_y = (bounds.start_y + bounds.end_y) / 2;
            createRect_recursive(image, {bounds.start_x, bounds.start_y, mid_x, mid_y});
            createRect_recursive(image, {bounds.start_x, mid_y + 1, mid_x, bounds.end_y});
            createRect_recursive(image, {mid_x + 1, bounds.start_y, bounds.end_x, mid_y});
            createRect_recursive(image, {mid_x + 1, mid_y + 1, bounds.end_x, bounds.end_y});
        }
    }
    uint32_t m_width;
    uint32_t m_height;
};

// Wraps BitImage into a 2d segment tree with optimized set(), get() and canBlit() functions
// Every node of tree contains 2 bits:
//   First bit indicates if children of the node have 1s in them
//   Second bit indicates if both of the children are filled.
// Has slow insertion that guarantees fast query.
class BitImage2dTree
{
public:
    BitImage2dTree(uint32_t width, uint32_t height) : m_width(width), m_height(height) {
        // every node in segment tree requires 2 bits
        m_bitImageOffset_x = (calculateOffset(m_width) + 1) * 2;
        m_bitImageOffset_y = calculateOffset(m_height) + 1;
//        m_data.resize(m_bitImageOffset_x + m_width * 2, m_bitImageOffset_y + m_height, true);
        m_data.resize(m_bitImageOffset_x * 2, m_bitImageOffset_y * 2, true);
    }

    uint32_t width() const { return m_width; }
    uint32_t height() const { return m_height; }

    void set(uint32_t x, uint32_t y) {
        fill(x, y, x, y);
    }

    bool get(uint32_t x, uint32_t y) const {
        return !canBlit(x, y, x, y);
    }

    void fill(uint32_t start_x, uint32_t start_y,
              uint32_t end_x, uint32_t end_y) {
        XA_DEBUG_ASSERT(start_x >= 0);
        XA_DEBUG_ASSERT(start_y >= 0);

        XA_DEBUG_ASSERT(start_x <= end_x);
        XA_DEBUG_ASSERT(start_y <= end_y);

        XA_DEBUG_ASSERT(end_x < m_width);
        XA_DEBUG_ASSERT(end_y < m_height);

        fill_recursive_y(1,
                         start_x, start_y, end_x, end_y,
                         0, m_height - 1);
    }

    void fill(const BitImageOpt &imageOpt, uint32_t offset_x, uint32_t offset_y) {
        for (int i = 0; i < imageOpt.m_data.size(); ++i) {
            const BitImageOpt::Rectangle &rect = imageOpt.m_data[i];
            fill(offset_x + rect.start_x, offset_y + rect.start_y, offset_x + rect.end_x, offset_y + rect.end_y);
        }
    }

    bool canBlit(uint32_t start_x, uint32_t start_y,
                 uint32_t end_x, uint32_t end_y) const {
        XA_DEBUG_ASSERT(start_x >= 0);
        XA_DEBUG_ASSERT(start_y >= 0);

        XA_DEBUG_ASSERT(end_x < m_width);
        XA_DEBUG_ASSERT(end_y < m_height);

        XA_DEBUG_ASSERT(start_x <= end_x);
        XA_DEBUG_ASSERT(start_y <= end_y);
        return canBlit_recursive_y(1,
                                   start_x, start_y, end_x, end_y,
                                   0, m_height - 1);
    }

    bool canBlit(const BitImageOpt &imageOpt, uint32_t offset_x, uint32_t offset_y) const {
        for (int i = 0; i < imageOpt.m_data.size(); ++i) {
            const BitImageOpt::Rectangle &rect = imageOpt.m_data[i];
            if (!canBlit(offset_x + rect.start_x, offset_y + rect.start_y, offset_x + rect.end_x, offset_y + rect.end_y))
                return false;
        }
        return true;
    }

    static uint32_t calculateOffset(uint32_t size) {
        return nextPowerOfTwo(size);
//        uint32_t sum = 0;
//        while (size > 1) {
//            size = (size + 1) / 2;
//            sum += size;
//        }
//        return sum;
    }

    void decompressInto(BitImage *image) const {
        XA_DEBUG_ASSERT(image != nullptr);
        image->resize(m_width, m_height, true);
        for (int y = 0; y < m_height; ++y) {
            for (int x = 0; x < m_width; ++x) {
                if (get(x, y))
                    image->set(x, y);
            }
        }
    }

private:
    void fill_recursive_y(uint32_t vert_y,
                          uint32_t start_x, uint32_t start_y, uint32_t end_x, uint32_t end_y,
                          uint32_t vert_start_y, uint32_t vert_end_y) {
        if (start_x > end_x || start_y > end_y)
            return;
        if (vert_end_y < start_y || vert_start_y > end_y)
            return;

        uint32_t vert_x = 1;


//        if (start_y != vert_start_y || end_y != vert_end_y) {
        if (vert_start_y != vert_end_y) {
            uint32_t mid_y = (vert_start_y + vert_end_y) / 2;
            fill_recursive_y(vert_y * 2,
                             start_x, start_y, end_x, min(end_y, mid_y),
                             vert_start_y, mid_y);
            fill_recursive_y(vert_y * 2 + 1,
                             start_x, max(start_y, mid_y + 1), end_x, end_y,
                             mid_y + 1, vert_end_y);
        }

        fill_recursive_x(vert_x, vert_y,
                         start_x, start_y, end_x, end_y,
                         0, vert_start_y, m_width - 1, vert_end_y);
    }

    void fill_recursive_x(uint32_t vert_x, uint32_t vert_y,
                          uint32_t start_x, uint32_t start_y, uint32_t end_x, uint32_t end_y,
                          uint32_t vert_start_x, uint32_t vert_start_y,
                          uint32_t vert_end_x, uint32_t vert_end_y) {
        if (start_x > end_x || start_y > end_y)
            return;
        if (vert_end_x < start_x || vert_start_x > end_x)
            return;

        set_data(vert_x, vert_y);
        if (start_x <= vert_start_x && end_x >= vert_end_x) {
            if (start_y <= vert_start_y && end_y >= vert_end_y) {
                XA_DEBUG_ASSERT(!get_full(vert_x, vert_y));
                set_full(vert_x, vert_y);
            }
//            // else there's nothing we can do
//            return;
        }

        // leave on last level
        if (vert_start_x == vert_end_x)
            return;
        uint32_t mid_x = (vert_start_x + vert_end_x) / 2;

        fill_recursive_x(vert_x * 2, vert_y,
                         start_x, start_y, min(end_x, mid_x), end_y,
                         vert_start_x, vert_start_y, mid_x, vert_end_y);
        fill_recursive_x(vert_x * 2 + 1, vert_y,
                         max(start_x, mid_x + 1), start_y, end_x, end_y,
                         mid_x + 1, vert_start_y, vert_end_x, vert_end_y);
    }

    bool canBlit_recursive_y(uint32_t vert_y,
                             uint32_t start_x, uint32_t start_y,
                             uint32_t end_x, uint32_t end_y,
                             uint32_t vert_start_y, uint32_t vert_end_y) const {
        if (start_x > end_x || start_y > end_y)
            return true;
        if (vert_end_y < start_y || vert_start_y > end_y)
            return true;

        uint32_t vert_x = 1;
        bool data = get_data(vert_x, vert_y);
        bool is_full = get_full(vert_x, vert_y);
        if (is_full)
            return false;
        if (!data) {
            return true;
        }

//        if (!canBlit_recursive_x(vert_x, vert_y, start_x, start_y, end_x, end_y, 0, m_width - 1, vert_start_y, vert_end_y))
//            return false;

        if (start_y == vert_start_y && end_y == vert_end_y) {
//            return true;
            return canBlit_recursive_x(vert_x, vert_y, start_x, start_y, end_x, end_y, 0, m_width - 1, vert_start_y, vert_end_y);
        }

        uint32_t mid_y = (vert_start_y + vert_end_y) / 2;

        return (canBlit_recursive_y(vert_y * 2,
                                    start_x, start_y, end_x, min(end_y, mid_y),
                                    vert_start_y, mid_y)
                && canBlit_recursive_y(vert_y * 2 + 1,
                                       start_x, max(start_y, mid_y + 1), end_x, end_y,
                                       mid_y + 1, vert_end_y));
    }

    bool canBlit_recursive_x(uint32_t vert_x, uint32_t vert_y,
                             uint32_t start_x, uint32_t start_y,
                             uint32_t end_x, uint32_t end_y,
                             uint32_t vert_start_x, uint32_t vert_end_x,
                             uint32_t vert_start_y, uint32_t vert_end_y) const {
        if (start_x > end_x || start_y > end_y)
            return true;
        if (vert_end_x < start_x || vert_start_x > end_x)
            return true;

        bool data = get_data(vert_x, vert_y);
        bool is_full = get_full(vert_x, vert_y);
        if (is_full)
            return false;
        if (!data) {
            return true;
        }

        if (start_x == vert_start_x && end_x == vert_end_x) {
            if (start_y == vert_start_y && end_y == vert_end_y) {
                // exact vert, has data in it
                return false;
            }
            // non-exact vert
            return true;
        }

        uint32_t mid_x = (vert_start_x + vert_end_x) / 2;

        return (canBlit_recursive_x(vert_x * 2, vert_y,
                                    start_x, start_y, min(end_x, mid_x), end_y,
                                    vert_start_x, mid_x,
                                    vert_start_y, vert_end_y)
                && canBlit_recursive_x(vert_x * 2 + 1, vert_y,
                                       max(start_x, mid_x + 1), start_y, end_x, end_y,
                                       mid_x + 1, vert_end_x,
                                       vert_start_y, vert_end_y));
    }

    inline bool get_data(uint32_t vert_x, uint32_t vert_y) const {
        return m_data.get(vert_x * 2, vert_y);
    }

    inline bool get_full(uint32_t vert_x, uint32_t vert_y) const {
        return m_data.get(vert_x * 2 + 1, vert_y);
    }

    inline void set_data(uint32_t vert_x, uint32_t vert_y) {
        m_data.set(vert_x * 2, vert_y);
    }

    inline void set_full(uint32_t vert_x, uint32_t vert_y) {
        m_data.set(vert_x * 2 + 1, vert_y);
    }

public:
    BitImage m_data;
    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_bitImageOffset_x;
    uint32_t m_bitImageOffset_y;
};

}
#endif //XATLAS_UVATLAS_COMPARISON_TEST_COMPRESSOR_H
