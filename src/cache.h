#pragma once

#include <type_traits>

#include "librandom.h"

template <typename T = uint32_t> 
struct cache_t
{
    static_assert(std::is_integral_v<T>, "Unsupported type!");

  public:
    inline cache_t(const T v = 0) : _cache(v)
    {
    }

  public:
    inline T value() const
    {
        return _cache;
    }

  public:
    inline T check(T idx, const uint8_t size)
    {
        check_all(size);
        while (!check_cache(static_cast<uint8_t>(idx), size)) {
            if (++idx >= size)
                idx = 0;
        }
        if (check_all(size))
            check_cache(static_cast<uint8_t>(idx), size);
        return (idx);
    }
  public:
    inline int check_all(const uint8_t size)
    {
        const T nall = static_cast<T>((uint64_t(-1) << size));
        const T all = ~nall;            
        if ((all & _cache) != all) 
            return (false);
        _cache &= nall;
        return (true);
    }
    inline int check_cache(const uint8_t idx, const uint8_t size)
    {
        const T mask =
            (static_cast<T>((uint64_t(1) << idx)) & ~(static_cast<T>(uint64_t(-1) << size)));
        if (_cache & mask)
            return (false);
        _cache |= mask;
        return (true);
    }
private:
    T _cache;
};

typedef cache_t<uint32_t> cache;