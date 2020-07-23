#ifndef JWUTIL_FUNCDERIVATIVES_H
#define JWUTIL_FUNCDERIVATIVES_H

#include <type_traits>

#include "multidimmatrix.h"
#include "fastmath.h"

namespace jw_util
{

class _FuncDerivatiesBaseEmpty {};

template <unsigned int vars, unsigned int derivatives, typename BaseType = float>
class FuncDerivatives : public std::conditional<derivatives != 0, FuncDerivatives<vars, derivatives - 1, BaseType>, _FuncDerivatiesBaseEmpty>::type
{
    template <unsigned int, unsigned int, typename>
    friend class FuncDerivatives;

public:
    FuncDerivatives()
    {
        matrix.set_zero();
    }

    template <unsigned int d>
    MultiDimMatrix<vars, d, BaseType> &get_deriv()
    {
        static_assert(d <= derivatives, "FuncDerivatives<vars, derivatives>::get_deriv<d>(): d must not be greater than derivatives");
        return FuncDerivatives<vars, d, BaseType>::matrix;
    }

    template <unsigned int d>
    const MultiDimMatrix<vars, d, BaseType> &get_deriv() const
    {
        static_assert(d <= derivatives, "FuncDerivatives<vars, derivatives>::get_deriv<d>(): d must not be greater than derivatives");
        return FuncDerivatives<vars, d, BaseType>::matrix;
    }


private:
    MultiDimMatrix<vars, derivatives, BaseType> matrix;
};

}

#endif // JWUTIL_FUNCDERIVATIVES_H
