#pragma once

#include "librandom.h"

template <typename holder_t = uint32_t> 
struct cache_t
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
    inline holder_t check(holder_t idx, const uint8_t size)
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
        const holder_t nall = static_cast<holder_t>((uint64_t(-1) << size));
        const holder_t all = ~nall;            
        if ((all & _cache) != all) 
            return (false);
        _cache &= nall;
        return (true);
    }
    inline int check_cache(const uint8_t idx, const uint8_t size)
    {
        const holder_t mask =
            (static_cast<holder_t>((uint64_t(1) << idx)) & ~(static_cast<holder_t>(uint64_t(-1) << size)));
        if (_cache & mask)
            return (false);
        _cache |= mask;
        return (true);
    }
private:
    holder_t _cache;
};

typedef cache_t<uint32_t> cache;