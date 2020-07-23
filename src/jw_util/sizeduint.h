#ifndef JWUTIL_SIZEDUINT_H
#define JWUTIL_SIZEDUINT_H

#include <limits.h>
#include <type_traits>
#include <array>

namespace jw_util
{

template <unsigned int bits>
class SizedUInt
{
private:
    typedef std::uint32_t DataType;
    typedef std::uint64_t OverflowDataType;
    static constexpr unsigned int size = bits / 32;

    static_assert(sizeof(DataType) * CHAR_BIT == 32, "The number of bits in DataType is not 32");
    static_assert(sizeof(OverflowDataType) * CHAR_BIT == 64, "The number of bits in OverflowDataType is not 64");
    static_assert(bits % 32 == 0, "The number of bits must be a multiple of 32");

public:
    template <unsigned int op_a_bits, unsigned int op_b_bits>
    class Operation
    {
    public:
        enum Operator {OpAdd, OpSub, OpMul, OpDiv, OpMod};

        Operation(const SizedUInt<op_a_bits> &op_a, const Operator op_type, const SizedUInt<op_b_bits> &op_b)
            : op_a(op_a)
            , op_type(op_type)
            , op_b(op_b)
        {}

    private:
        const SizedUInt<op_a_bits> &op_a;
        const Operator op_type;
        const SizedUInt<op_b_bits> &op_b;

        template <unsigned int res_bits>
        void eval(SizedUInt<res_bits> &res)
        {
            switch (op_type)
            {
            case OpAdd:
                eval_add(res);
                break;

            case OpSub:
                eval_sub(res);
                break;

            case OpMul:
                eval_mul(res);
                break;

            case OpDiv:
            case OpMod:
                break;
            }
        }

        template <unsigned int res_bits>
        void eval_add(SizedUInt<res_bits> &res)
        {
            OverflowDataType carry = 0;
            for (unsigned int i = 0; i < res.size; i++)
            {
                if (i < op_a.size) {carry += op_a.data[i];}
                if (i < op_b.size) {carry += op_b.data[i];}

                res.data[i] = static_cast<DataType>(carry);
                carry >>= 32;
            }
        }

        template <unsigned int res_bits>
        void eval_sub(SizedUInt<res_bits> &res)
        {
            OverflowDataType carry = 0;
            for (unsigned int i = 0; i < res.size; i++)
            {
                if (i < op_a.size) {carry -= op_a.data[i];}
                if (i < op_b.size) {carry -= op_b.data[i];}

                res.data[i] = static_cast<DataType>(carry);
                carry >>= 32;
            }
        }

        template <unsigned int res_bits>
        void eval_mul(SizedUInt<res_bits> &res)
        {
            res.zero();

            for (unsigned int i = 0; i < op_a.size; i++)
            {
                for (unsigned int j = 0; j < op_b.size; j++)
                {
                    res.add_shifted(i + j, op_a.data[i] * op_b.data[j]);
                }
            }
        }
    };

    SizedUInt()
    {}

    void zero()
    {
        data.fill(0);
    }

    template <typename SetType>
    void set_integral(SetType val)
    {
        static_assert(std::is_integral<SetType>::value, "set_integral argument must be integral");
        assert(val >= 0);

        for (unsigned int i = 0; i < size; i++)
        {
            data[i] = val;
            val >>= 32;
        }
    }

    void set_raw(const void *val, unsigned int len_bytes)
    {
        len_bytes = min(len_bytes, size * sizeof(DataType));

        void *dst = static_cast<void*>(&data[0]);
        memcpy(dst, val, len_bytes);
        memset(dst + len_bytes, 0, size - len_bytes);
    }

    template <unsigned int other_bits>
    Operation<bits, other_bits> operator+(const SizedUInt<other_bits> &other) const
    {
        return Operation<bits, other_bits>(*this, Operation::OpAdd, other);
    }

    template <unsigned int other_bits>
    Operation<bits, other_bits> operator-(const SizedUInt<other_bits> &other) const
    {
        return Operation<bits, other_bits>(*this, Operation::OpSub, other);
    }

    template <unsigned int other_bits>
    Operation<bits, other_bits> operator*(const SizedUInt<other_bits> &other) const
    {
        return Operation<bits, other_bits>(*this, Operation::OpMul, other);
    }

    template <unsigned int other_bits>
    SizedUInt<bits> &operator=(const SizedUInt<other_bits>& other)
    {
        assert(this != &other);

        for (unsigned int i = 0; i < size; i++)
        {
            data[i] = i < other.size ? other.data[i] : 0;
        }

        return *this;
    }

    template <unsigned int op_a_bits, unsigned int op_b_bits>
    SizedUInt<bits> &operator=(const Operation<op_a_bits, op_b_bits> other)
    {
        other.eval(*this);
        return *this;
    }

private:
    std::array<DataType, size> data;

    void add_shifted(unsigned int shift, OverflowDataType add)
    {
        OverflowDataType carry = add;
        for (unsigned int i = shift; carry && i < size; i++)
        {
            carry += data[i];
            data[i] = carry;
            carry >>= 32;
        }
    }
};

}

#endif // JWUTIL_SIZEDUINT_H
