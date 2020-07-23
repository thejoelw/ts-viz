#ifndef JWUTIL_BITITERATOR_H
#define JWUTIL_BITITERATOR_H

#include "huffmanmodel.h"

namespace jw_util
{

template <typename DataType = unsigned int>
class BitInterface
{
public:
    BitInterface()
    {}

    BitInterface(DataType *data)
        : data(data)
        , cur_bit(0)
    {}

    template <typename NumberType>
    NumberType read_number_raw(unsigned int bits)
    {
        static_assert(std::is_integral<NumberType>::value, "Unexpected template type, must be integral");
        static_assert(std::is_unsigned<NumberType>::value, "Unexpected template type, must be unsigned");

        NumberType num = 0;

        for (unsigned int i = 0; i < bits; i++)
        {
            num |= static_cast<NumberType>(read_bit()) << i;
        }

        return num;
    }

    template <unsigned int options>
    unsigned int read_number_huffman(const HuffmanModel<options> &model)
    {
        typename HuffmanModel<options>::Reader reader(model);
        while (reader.needs_bit())
        {
            reader.recv_bit(read_bit());
        }

        return reader.get_result();
    }

    bool read_bit()
    {
        load_word();

        bool val = (*data >> cur_bit) & 1;
        cur_bit++;
        return val;
    }

    template <typename NumberType>
    void write_number_raw(NumberType num, unsigned int bits)
    {
        static_assert(std::is_integral<NumberType>::value, "Unexpected template type, must be integral");
        static_assert(std::is_unsigned<NumberType>::value, "Unexpected template type, must be unsigned");

        for (unsigned int i = 0; i < bits; i++)
        {
            write_bit(num & 1);
            num >>= 1;
        }
    }

    template <unsigned int options>
    void write_number_huffman(const HuffmanModel<options> &model, unsigned int num)
    {
        write_number_huffman(HuffmanModel<options>::Writer(model, num));
    }
    template <unsigned int options>
    void write_number_huffman(typename HuffmanModel<options>::Writer writer)
    {
        while (writer.has_bit())
        {
            write_bit(writer.get_bit());
            writer.next_bit();
        }
    }

    void write_bit(bool bit)
    {
        load_word();

        *data |= bit << cur_bit;
        cur_bit++;
    }

    void advance_bits(unsigned int bits) const
    {
        data += bits / bits_per_element;
        cur_bit = bits % bits_per_element;
    }

    unsigned int get_element_delta(const DataType *start) const
    {
        return data - start + (cur_bit ? 1 : 0);
    }
    unsigned int get_bit_delta(const DataType *start) const
    {
        return (data - start) * bits_per_element + cur_bit;
    }

private:
    static constexpr unsigned int bits_per_element = sizeof(DataType) * CHAR_BIT;
    
    DataType *data;
    unsigned int cur_bit;

    void load_word()
    {
        if (cur_bit == bits_per_element)
        {
            data++;
            cur_bit = 0;
        }
    }
};

}

#endif // JWUTIL_BITITERATOR_H
