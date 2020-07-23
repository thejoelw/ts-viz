#ifndef JWUTIL_INTSHUFFLEITERATOR_H
#define JWUTIL_INTSHUFFLEITERATOR_H

#include "linearfeedbackshiftregister.h"
#include "fastmath.h"

namespace jw_util
{

template <typename NumberType>
class IntShuffleIterator
{
public:
    enum State {SequenceStart = 0, SequenceMiddle = 1};

    IntShuffleIterator()
    {}

    IntShuffleIterator(NumberType size)
        : lfsr(FastMath::log2<NumberType>(size) + 1, 1)
        , size(size)
    {
        assert(size > 0);
    }

    State get_state() const
    {
        return lfsr.get() == 1 ? SequenceStart : SequenceMiddle;
    }

    NumberType get() const
    {
        return lfsr.get() - 1;
    }

    void next()
    {
        do
        {
            lfsr.next();
        } while (lfsr.get() > size);
    }

private:
    LinearFeedbackShiftRegister<NumberType> lfsr;
    NumberType size;
};

}

#endif // JWUTIL_INTSHUFFLEITERATOR_H
