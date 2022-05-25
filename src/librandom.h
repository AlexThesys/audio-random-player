#pragma once

#include <assert.h>
#include <stdint.h>

namespace librandom
{

class simple
{
  private:
    volatile int64_t holdrand;

  public:
    inline simple(const int64_t _seed = 1) : holdrand(_seed)
    {
    }

    // C5220
    simple(const simple &) = delete;
    simple &operator=(const simple &) = delete;
    simple(simple &&) = delete;
    simple &operator=(simple &&) = delete;

    inline void seed(const int64_t val)
    {
        holdrand = val;
    }
    inline int64_t seed() const
    {
        return holdrand;
    }
    inline int32_t max_i() const
    {
        return 32767;
    }

    inline int32_t i()
    {
        return (((holdrand = holdrand * 214013L + 2531011L) >> 16) & 0x7fff);
    }

    inline int32_t i(int32_t max)
    {
        assert(max);
        return i() % max;
    }
    inline int32_t i(int32_t min, int32_t max)
    {
        return min + i(max - min);
    }
    inline int32_t is(int32_t range)
    {
        return i(-range, range);
    }
    inline int32_t is(int32_t range, int32_t offs)
    {
        return offs + is(range);
    }

    inline float max_f()
    {
        return 32767.f;
    }
    inline float f()
    {
        return float(i()) / max_f();
    }
    inline float f(float max)
    {
        return f() * max;
    }
    inline float f(float min, float max)
    {
        return min + f(max - min);
    }
    inline float fs(float range)
    {
        return f(-range, range);
    }
    inline float fs(float range, float offs)
    {
        return offs + fs(range);
    }
};

template <typename holder_t = uint32_t> struct cache_t
{
  public:
    inline cache_t(const holder_t v = 0) : _cache(v)
    {
    }

  public:
    inline holder_t value() const
    {
        return _cache;
    }

  public:
    // primary
    inline holder_t check(holder_t idx, const uint8_t size)
    {
        // priori fullness
        check_all(size);
        // routine
        while (!check_cache((uint8_t)idx, size)) {
            if (++idx >= size)
                idx = 0;
        }
        // posteriori fullness
        if (check_all(size))
            check_cache((uint8_t)idx, size); // mark current as passed
        return (idx);
    }

  public:
    // subcache
    inline holder_t subcache(const uint8_t size) const
    {
        const holder_t subcache = (_cache >> size);
        return (subcache);
    }
    inline void apply_subcache(const holder_t subcache, const uint8_t size)
    {
        _cache |= (subcache << size);
    }

  public:
    // aux
    inline int check_all(const uint8_t size)
    {
        const holder_t nall = this->nall(size); // 1111111...0 * size
        const holder_t all = ~nall;             // 0000000...1 * size
        if ((all & _cache) != all)              // check if its full
            return (false);
        _cache &= nall; // clear
        return (true);
    }
    inline int check_cache(const uint8_t idx, const uint8_t size)
    {
        const holder_t mask = (holder_t)((uint64_t(1) << idx) & all(size));
        if (_cache & mask)
            return (false);
        _cache |= mask;
        return (true);
    }

  public:
    inline int check_only(const uint8_t idx, const uint8_t size) const
    {
        const holder_t mask = (holder_t)((uint64_t(1) << idx) & all(size));
        return !(_cache & mask);
    }

  private:
    inline holder_t nall(const uint8_t size) const
    {
        return (holder_t)(uint64_t(-1) << size); // 1111111...0 * size
    }
    inline holder_t all(const uint8_t size) const
    {
        return (~nall(size)); // 0000000...1 * size
    }

    holder_t _cache;
};

typedef cache_t<uint32_t> cache;
} // namespace librandom