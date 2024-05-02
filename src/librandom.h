#pragma once

#include <assert.h>
#include <stdint.h>

namespace librandom
{
// randu algorithm
class randu
{
  private:
    volatile int64_t _rnd;

  public:
    inline randu(const int64_t _seed = 1) : _rnd(_seed)
    {
    }

    // C5220
    randu(const randu&) = delete;
    randu&operator=(const randu&) = delete;
    randu(randu&&) = delete;
    randu &operator=(randu&&) = delete;

    inline void seed(const int64_t val)
    {
        _rnd = val;
    }
    inline int64_t seed() const
    {
        return _rnd;
    }
    inline int32_t max_i() const
    {
        return 32767;
    }

    inline int32_t i()
    {
        return (((_rnd = _rnd * 214013L + 2531011L) >> 16) & 0x7fff);
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

    inline float max_fp()
    {
        return 32767.f;
    }
    inline float fp()
    {
        return float(i()) / max_fp();
    }
    inline float fp(float max)
    {
        return fp() * max;
    }
    inline float fp(float min, float max)
    {
        return min + fp(max - min);
    }
    inline float fp_s(float range)
    {
        return fp(-range, range);
    }
    inline float fp_s(float range, float offs)
    {
        return offs + fp_s(range);
    }
};
} // namespace librandom