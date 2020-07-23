#ifndef JWUTIL_MULTIDIMMATRIX_H
#define JWUTIL_MULTIDIMMATRIX_H

#include <assert.h>
#include <type_traits>
#include <array>

#include "fastmath.h"

#ifdef MULTIDIMMATRIX_INIT_GARBAGE
#include <cstdlib>
#endif

namespace jw_util
{

namespace
{
    enum Operator {
        OpSet,
        OpAdd,
        OpSub
    };
}

template <typename MultiDimMatrixType>
class _MultiDimMatrixBase
{
public:
    template <typename SetType>
    MultiDimMatrixType &operator=(const SetType &set)
    {
        MultiDimMatrixType &child = *static_cast<MultiDimMatrixType*>(this);
        child.template accumulate<OpSet, SetType>(set);
        return child;
    }

    template <typename AddType>
    MultiDimMatrixType &operator+=(const AddType &add)
    {
        MultiDimMatrixType &child = *static_cast<MultiDimMatrixType*>(this);
        child.template accumulate<OpAdd, AddType>(add);
        return child;
    }

    template <typename SubType>
    MultiDimMatrixType &operator-=(const SubType &sub)
    {
        MultiDimMatrixType &child = *static_cast<MultiDimMatrixType*>(this);
        child.template accumulate<OpSub, SubType>(sub);
        return child;
    }
};

template <unsigned int size, unsigned int dims, typename BaseType = float>
class MultiDimMatrix : public _MultiDimMatrixBase<MultiDimMatrix<size, dims, BaseType>>
{
    typedef _MultiDimMatrixBase<MultiDimMatrix<size, dims, BaseType>> ParentClass;

    friend ParentClass;

private:
    static_assert(dims != 0, "Special case MultiDimMatrix<size, 0> not handled by template specialization");

    typedef MultiDimMatrix<size, dims - 1, BaseType> ChildMatrixType;
    typedef std::array<ChildMatrixType, size> DataType;
    DataType data;

public:
    MultiDimMatrix()
    {
        //static_assert(sizeof(MultiDimMatrix<size, dims>) == FastMath::pow<dims>(size) * sizeof(float), "Unexpected size for MultiDimMatrix");
    }

    void set_zero()
    {
        for (unsigned int i = 0; i < size; i++)
        {
            data[i].set_zero();
        }
    }

    ChildMatrixType &operator[](unsigned int i)
    {
        assert(i < size);
        return data[i];
    }

    const ChildMatrixType &operator[](unsigned int i) const
    {
        assert(i < size);
        return data[i];
    }

    using ParentClass::operator =;
    using ParentClass::operator +=;
    using ParentClass::operator -=;

private:
    template <Operator op, typename OperandType>
    void accumulate(const OperandType &operand)
    {
        for (unsigned int i = 0; i < size; i++)
        {
            data[i].template accumulate<op>(operand[i]);
        }
    }
};

template <unsigned int size, typename BaseType>
class MultiDimMatrix<size, 0, BaseType> : public _MultiDimMatrixBase<MultiDimMatrix<size, 0, BaseType>>
{
    typedef _MultiDimMatrixBase<MultiDimMatrix<size, 0, BaseType>> ParentClass;

    friend ParentClass;
    friend class MultiDimMatrix<size, 1, BaseType>;

private:
    BaseType data;

public:
    MultiDimMatrix()
    {
#ifdef MULTIDIMMATRIX_INIT_GARBAGE
        data = static_cast<BaseType>(rand());
#endif
    }

    void set_zero()
    {
        data = static_cast<BaseType>(0);
    }

    operator BaseType() const
    {
        return data;
    }

    using ParentClass::operator =;
    using ParentClass::operator +=;
    using ParentClass::operator -=;

private:
    template <Operator op, typename OperandType>
    void accumulate(const OperandType &operand)
    {
        switch (op)
        {
        case OpSet: data = operand; break;
        case OpAdd: data += operand; break;
        case OpSub: data -= operand; break;
        }
    }
};

}

#endif // JWUTIL_MULTIDIMMATRIX_H
