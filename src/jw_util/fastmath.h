#ifndef JWUTIL_FASTMATH_H
#define JWUTIL_FASTMATH_H

#include "intinverse.h"

#include <cmath>
#include <assert.h>
#include <type_traits>

namespace jw_util
{

class FastMath
{
public:
    enum SqrtImprecision {SqrtImpEither, SqrtImpGreater, SqrtImpLesser};

    template <unsigned int Iterations, SqrtImprecision imp = SqrtImpEither>
    static float sqrt(float x)
    {
        assert(x >= 0.0f);

        return ::sqrt(x);

        // TODO: Implement imprecision

        // Implementation taken from wikipedia: http://en.wikipedia.org/wiki/Methods_of_computing_square_roots

        /*
        unsigned int val_int = *reinterpret_cast<unsigned int*>(&x);

        val_int -= 1 << 23; // Subtract 2^m
        val_int >>= 1; // Divide by 2
        val_int += 1 << 29; // Add ((b + 1) / 2) * 2^m

        float y = *reinterpret_cast<float*>(&val_int);

        for (unsigned int i = 0; i < Iterations; i++)
        {
            y = (y + x / y) / 2.0;
        }

        return y;
        */

        /*
        The maximum errors where x < 1000.0 are around 511.99:
        Iterations=0 -> +1.372566700
        Iterations=1 -> +0.039249882
        Iterations=2 -> +0.000034897
        Iterations=3 -> +0.000001361

        The average error where x < 1000.0 is quite a bit lower:
        Iterations=0 -> 0.390017241
        Iterations=1 -> 0.006606600
        Iterations=2 -> 0.000003428
        Iterations=3 -> 0.000000510
        */
    }

    template <typename T>
    static signed int log2(T num)
    {
        static_assert(std::is_integral<T>::value || std::is_floating_point<T>::value, "Unknown argument type (not integral or floating point) for FastMath::log2");

        if (std::is_integral<T>::value)
        {
            static_assert(std::is_unsigned<T>::value, "Cannot call FastMath::log2 with a signed integer argument");

            signed int res = 0;
            while (num >>= 1) {res++;}
            return res;
        }
        else if (std::is_floating_point<T>::value)
        {
            // TODO: make sure this code works
            assert(false);

            signed int result;
            std::frexp(num, &result);
            return result - 1;
        }
    }

    template <typename T>
    static constexpr signed int log2_constexpr(T num)
    {
        static_assert(std::is_integral<T>::value, "Unknown argument type (not integral) for constexpr FastMath::log2");

        static_assert(std::is_unsigned<T>::value, "Cannot call FastMath::log2 with a signed integer argument");

        signed int res = 0;
        while (num >>= 1) {res++;}
        return res;
    }

    template <typename T>
    static constexpr bool is_pow2(T num)
    {
        return ((num - 1) & num) == 0;
    }

    static constexpr unsigned int next_power_of_2(unsigned int v) {
        v--;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
        v++;
        return v;
    }

#ifdef FASTMATH_ENABLE_INTINVERSE_FLOAT
    template <typename T>
    static float int_inverse_float(T num)
    {
        return IntInverse<float, FASTMATH_INT_INVERSE_MAX>::inverse<T>(num);
    }
#endif

#ifdef FASTMATH_ENABLE_INTINVERSE_DOUBLE
    template <typename T>
    static float int_inverse_double(T num)
    {
        return IntInverse<double, FASTMATH_INT_INVERSE_MAX>::inverse<T>(num);
    }
#endif

    template <typename T, unsigned int exp>
    static T pow(T num)
    {
        if (exp == 0) {return static_cast<T>(1);}
        if (exp == 1) {return num;}

        float half = pow<exp / 2, T>(num);
        float full = half * half;
        if (exp % 2) {full *= num;}
        return full;
    }

    template <typename T>
    static constexpr T factorial(T num)
    {
        assert(num >= 0);

        if (num == 0) {return static_cast<T>(1);}
        if (num == 1) {return static_cast<T>(1);}

        return num * factorial<T>(num - 1);
    }

    template <typename T>
    static constexpr T permutations(T n, T k)
    {
        assert(n >= 0);
        assert(k >= 0);
        assert(k <= n);

        T res = static_cast<T>(1);

        T i = n - k;
        while (i < n)
        {
            i++;
            res *= i;
        }

        return res;
    }

    template <typename T>
    static constexpr T combinations(T n, T k)
    {
        assert(n >= 0);
        assert(k >= 0);
        assert(k <= n);

        T num = static_cast<T>(1);
        T den = static_cast<T>(1);

        T i = k;
        while (i < n)
        {
            den *= n - i;
            i++;
            num *= i;
        }

        return num / den;
    }

    template <typename T>
    static T gcd(T a)
    {
        return a;
    }
    template <typename T, typename... Args>
    static T gcd(T a, Args... args)
    {
        auto b = gcd(args...);

        while (b != 0)
        {
            unsigned tmp = a % b;
            a = b;
            b = tmp;
        }

        return a;
    }

    template <typename T>
    static T lcm(T a)
    {
        return a;
    }
    template <typename T, typename... Args>
    static T lcm(T a, Args... args)
    {
        auto b = lcm(args...);

        while (true)
        {
            if (a == 0) {return b;}
            b %= a;
            if (b == 0) {return a;}
            a %= b;
        }
    }

    template <typename T>
    static constexpr T round_up(T num, T multiple)
    {
        return ((num + multiple - 1) / multiple) * multiple;
    }

    template <typename T>
    static constexpr T round_down(T num, T multiple)
    {
        return (num / multiple) * multiple;
    }

    template <typename T>
    static constexpr T div_ceil(T num, T div)
    {
        return (num + div - 1) / div;
    }
};

/*
template <unsigned int Iterations>
btScalar fastsqrt(btScalar y)
{
    double x, z, tempf;
    unsigned long *tfptr = ((unsigned long *)&tempf) + 1;

    tempf = y;
    *tfptr = (0xbfcdd90a - *tfptr)>>1; // estimate of 1/sqrt(y)
    x =  tempf;
    z =  y*btScalar(0.5);

    for (unsigned int i = 0; i < Iterations; i++)
    {
        x = (btScalar(1.5)*x)-(x*x)*(x*z);
    }
    return x*y;
}
*/

}

#endif // JWUTIL_FASTMATH_H
