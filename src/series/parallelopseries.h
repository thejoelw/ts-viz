#pragma once

#include "jw_util/hash.h"

#include "series/series.h"

namespace series {

template <typename ElementType, unsigned int numArgs, typename OperatorType>
class ParallelOpSeries : public Series<ElementType> {
public:
    ParallelOpSeries(tf::Taskflow &taskflow, OperatorType op, Series<ElementType> *args[numArgs])
        : Series<ElementType>(taskflow.emplace([this](){this->compute();}))
        , op(op)
        , args(args)
    {
        for (unsigned int i = 0; i < numArgs; i++) {
            args[i]->task.precede(this->task);
        }
    }

    void request(std::size_t begin, std::size_t end) {
        if (begin < this->requestedBegin) {
            this->requestedBegin = begin;
        }
        if (end > this->requestedEnd) {
            this->requestedEnd = end;
        }

        for (unsigned int i = 0; i < numArgs; i++) {
            args[i].request(begin, end);
        }
    }

    void compute() {
        ElementType *ptrs[numArgs];
        ElementType *dst;

        if (this->requestedBegin < this->dataOffset) {
            std::vector<ElementType> tmp;
            tmp.resize(std::max(this->data.size() + this->dataOffset, this->requestedEnd) - this->requestedBegin);
            std::copy(this->data.cbegin(), this->data.cend(), tmp.begin() + (this->dataOffset - this->requestedBegin));
            tmp.swap(this->data);

            for (unsigned int i = 0; i < numArgs; i++) {
                ptrs[i] = args[i]->getData(this->requestedBegin);
            }
            dst = this->getData(this->requestedBegin);

            for (std::size_t i = this->requestedBegin; i < this->dataOffset; i++) {
                *dst++ = callOp(ptrs, std::make_index_sequence<numArgs>{});
                for (unsigned int j = 0; j < numArgs; j++) {
                    ptrs[j]++;
                }
            }

            this->dataOffset = this->requestedBegin;
        }

        std::size_t prevSize = this->data.size() + this->dataOffset;
        if (this->requestedEnd > prevSize) {
            this->data.resize(this->requestedEnd - this->dataOffset);

            for (unsigned int i = 0; i < numArgs; i++) {
                ptrs[i] = args[i]->getData(prevSize);
            }
            dst = this->getData(prevSize);

            for (std::size_t i = prevSize; i < this->requestedEnd; i++) {
                *dst++ = callOp(ptrs, std::make_index_sequence<numArgs>{});
                for (unsigned int j = 0; j < numArgs; j++) {
                    ptrs[j]++;
                }
            }
        }
    }

private:
    OperatorType op;

    Series<ElementType> *args[numArgs];

    template <std::size_t... Idx>
    ElementType callOp(ElementType *ptrs[numArgs], std::index_sequence<Idx...>) {
        return op(*ptrs[Idx]...);
    }
};

}
