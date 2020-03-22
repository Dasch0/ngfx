#ifndef __UTIL_HEADER
#define __UTIL_HEADER

#include <cstdint>

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)


template <typename T, uint16_t max_size>
class Table
{
public:
    uint16_t count;
    int16_t status;
    uint32_t flags;
    T data[max_size];

    uint16_t add()
    {
        if (unlikely(count>max_size))
            // TODO: better error handling
            status = -1;

        return count++;
    }

    T *get(uint16_t index)
    {
        return &data[index];
    }

    Table(void) : count(0), status(0), flags(0) {}
    ~Table(void) {}
};

#endif //__UTIL_HEADER
