#ifndef JWUTIL_INTINVERSE_H
#define JWUTIL_INTINVERSE_H

#include <assert.h>
#include <array>
#include <type_traits>

namespace jw_util
{

template <typename ResultType, unsigned int max>
class IntInverse
{
public:
    template <typename InputType>
    static ResultType inverse(InputType num)
    {
        static_assert(std::is_integral<InputType>::value, "Cannot call IntInverse::inverse with non-integral argument");
        static_assert(std::is_unsigned<InputType>::value, "Cannot call IntInverse::inverse with a signed integer argument");
        assert(num > 0);
        assert(num < max);
        return int_inverses[num];
    }

private:
    static const std::array<ResultType, max> int_inverses;

    static std::array<ResultType, max> calc_int_inverses()
    {
        std::array<ResultType, max> res;
        for (unsigned int i = 0; i < max; i++)
        {
            res[i] = static_cast<ResultType>(1) / i;
        }
        return res;
    }
};

template <typename ResultType, unsigned int max>
const std::array<ResultType, max> IntInverse<ResultType, max>::int_inverses = IntInverse<ResultType, max>::calc_int_inverses();

}

#endif // JWUTIL_INTINVERSE_H
