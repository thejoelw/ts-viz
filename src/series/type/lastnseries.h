#pragma once

#include "series/dataseries.h"
#include "series/invalidparameterexception.h"

namespace series {

template <typename ElementType, typename OperatorType, typename DataArg, typename CountArg>
class LastNSeries : public DataSeries<ElementType> {
private:
    class HeapToRangeCache {
    public:
        struct Range {
            unsigned int start;
            unsigned int end;
            unsigned int sizeLog2;

            void assertValid() const {
                assert(sizeLog2 > 0); // We don't handle ranges of size 1
                assert(sizeLog2 <= CHUNK_SIZE_LOG2);
                assert(start + (1u << sizeLog2) == end);
            }
        };

        static const HeapToRangeCache &getInstance() {
            jw_util::Thread::assert_main_thread(); // Only want to create one cache
            static thread_local HeapToRangeCache inst;
            return inst;
        }

        HeapToRangeCache() {
            unsigned int nextRange = 0;
            for (unsigned int i = 0; i < CHUNK_SIZE; i++) {
                Range range;
                range.end = i + 1;

                for (unsigned int j = 0; (range.end & (1u << j)) == 0; j++) {
                    range.sizeLog2 = j + 1;
                    range.start = range.end - (1u << range.sizeLog2);
                    range.assertValid();
                    assert(range2index(range) == nextRange);
                    ranges[nextRange] = range;
                    nextRange++;
                }
            }

            assert(nextRange == CHUNK_SIZE - 1);
        }

        Range index2range(unsigned int index) const {
            assert(index < CHUNK_SIZE - 1);
            assert(range2index(ranges[index]) == index);
            return ranges[index];
        }

        unsigned int range2index(Range range) const {
            range.assertValid();
            assert(range.end >= 2);
            assert(range.end - 2 >= __builtin_popcount(range.start));
            unsigned int res = range.end - __builtin_popcount(range.start) - 2;
            assert(res < CHUNK_SIZE - 1);
            return res;
        }

    private:
        Range ranges[CHUNK_SIZE - 1];
    };

    class ReductionHeap {
    public:
        ReductionHeap(const LastNSeries<ElementType, OperatorType, DataArg, CountArg> *lns)
            : lns(lns)
        {}

        Chunk<ElementType> *makeChunk(std::size_t chunkIndex) override {
            ChunkPtr<ElementType> chunk = lns->data.getChunk(chunkIndex);
            const HeapToRangeCache &heapToRangeCache = HeapToRangeCache::getInstance();
            return this->constructChunk([chunk = std::move(chunk), op = lns->op, &heapToRangeCache](ElementType *dst, unsigned int computedCount) -> unsigned int {
                unsigned int ccc = chunk->getComputedCount();
                while (true) {
                    // This could probably be optimized

                    typename HeapToRangeCache::Range range = heapToRangeCache.index2range(computedCount);
                    if (range.end > ccc) {
                        break;
                    }

                    assert(range.sizeLog2 > 0);
                    if (range.sizeLog2 == 1) {
                        dst[computedCount] = op(chunk->getElement(range.start), chunk->getElement(range.start + 1));
                    } else {
                        dst[computedCount] = op(dst[computedCount - (1u << (range.sizeLog2 - 1))], dst[computedCount - 1]);
                    }

                    computedCount++;
                }

                return computedCount;
            });
        }

    private:
        const LastNSeries<ElementType, OperatorType, DataArg, CountArg> *lns;
    };

public:
    LastNSeries(app::AppContext &context, OperatorType op, DataArg &data, CountArg &count, typename CountArg::ElementType maxCount)
        : DataSeries<ElementType>(context)
        , op(op)
        , data(data)
        , count(count)
        , maxCount(maxCount)
        , heap(this)
    {
        if (maxCount == 0) {
            throw series::InvalidParameterException("LastNSeries: maxCount must be at least one");
        }
    }

    Chunk<ElementType> *makeChunk(std::size_t chunkIndex) override {
        unsigned int numDataChunks = std::min<unsigned int>((maxCount + CHUNK_SIZE - 2) / CHUNK_SIZE, chunkIndex) + 1;
        std::vector<std::pair<ChunkPtr<typename DataArg::ElementType>, ChunkPtr<typename DataArg::ElementType>>> dataChunks;
        dataChunks.reserve(numDataChunks);
        for (unsigned int i = 0; i < numDataChunks; i++) {
            assert(i <= chunkIndex);
            dataChunks.emplace_back(data.getChunk(chunkIndex - i), data.heap.getChunk(chunkIndex - i));
        }

        ChunkPtr<typename CountArg::ElementType> countChunk = count.getChunk(chunkIndex);

        std::uint64_t nanEnd = maxCount - 1 > static_cast<std::uint64_t>(chunkIndex) * CHUNK_SIZE ? maxCount - 1 - static_cast<std::uint64_t>(chunkIndex) * CHUNK_SIZE : 0;

        return this->constructChunk([dataChunks = std::move(dataChunks), countChunk = std::move(countChunk), maxCount = maxCount, nanEnd](ElementType *dst, unsigned int computedCount) -> unsigned int {
            while (computedCount < nanEnd) {
                dst[computedCount++] = NAN;
            }

            unsigned int cccc = countChunk->getComputedCount();
            while (computedCount < cccc) {
                std::size_t count = std::max(typename CountArg::ElementType(1), std::min(countChunk->getElement(computedCount), maxCount));

                // TODO
                assert(false);
            }

            return computedCount;
        });
    }

private:
    OperatorType op;

    DataArg &data;
    CountArg &count;

    typename CountArg::ElementType maxCount;

    ReductionHeap heap;
};

}
