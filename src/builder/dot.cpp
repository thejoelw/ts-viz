#include "program/resolver.h"
#include "series/type/parallelcomputerseries.h"
#include "series/type/compseries.h"
#include "series/invalidparameterexception.h"

template <typename ArrItemType1, typename ArrItemType2>
std::size_t getSize(const program::ProgObjArray<ArrItemType1> &a, const program::ProgObjArray<ArrItemType2> &b) {
    if (a.getArr().size() != b.getArr().size()) {
        throw series::InvalidParameterException("dot: Array sizes must be equal!");
    }
    return a.getArr().size();
}

template <typename ElementType>
void declDotOp(app::AppContext &context, program::Resolver &resolver) {
    resolver.decl("dot", [&context](const program::ProgObjArray<double> a, const program::ProgObjArray<double> b){
        std::size_t size = getSize(a, b);
        ElementType sum = 0.0;
        for (std::size_t i = 0; i < size; i++) {
            sum += a.getArr()[i] * b.getArr()[i];
        }
        auto op = [sum](std::size_t i) {(void) i; return sum;};
        
        return new series::CompSeries<ElementType, decltype(op)>(context, op);
    });

    resolver.decl("dot", [&context](const program::ProgObjArray<ElementType> a, const program::ProgObjArray<series::DataSeries<ElementType> *> b){
        auto op = [size = getSize(a, b), a = a.getArr()](ElementType *dst, unsigned int computedCount, const std::vector<series::ChunkPtr<ElementType>> &b){
            assert(a.size() == size);
            assert(b.size() == size);

            unsigned int endCount = CHUNK_SIZE;
            for (const series::ChunkPtr<ElementType> &ptr : b) {
                unsigned int cc = ptr->getComputedCount();
                if (cc < endCount) {
                    endCount = cc;
                }
            }

            if (endCount <= computedCount) {
                return computedCount;
            }

            for (unsigned int j = computedCount; j < endCount; j++) {
                dst[j] = 0.0;
            }

            for (std::size_t i = 0; i < size; i++) {
                ElementType aEl = a[i];
                const series::ChunkPtr<ElementType> &bEl = b[i];
                for (unsigned int j = computedCount; j < endCount; j++) {
                    dst[j] += aEl * bEl->getElement(j);
                }
            }

            return endCount;
        };

        return new series::ParallelComputerSeries<ElementType, decltype(op), const program::ProgObjArray<series::DataSeries<ElementType> *>>(context, op, b);
    });

    resolver.decl("dot", [&context](const program::ProgObjArray<series::DataSeries<ElementType> *> a, const program::ProgObjArray<ElementType> b){
        auto op = [size = getSize(a, b), b = b.getArr()](ElementType *dst, unsigned int computedCount, const std::vector<series::ChunkPtr<ElementType>> &a){
            assert(a.size() == size);
            assert(b.size() == size);

            unsigned int endCount = CHUNK_SIZE;
            for (const series::ChunkPtr<ElementType> &ptr : a) {
                unsigned int cc = ptr->getComputedCount();
                if (cc < endCount) {
                    endCount = cc;
                }
            }

            if (endCount <= computedCount) {
                return computedCount;
            }

            for (unsigned int j = computedCount; j < endCount; j++) {
                dst[j] = 0.0;
            }

            for (std::size_t i = 0; i < size; i++) {
                const series::ChunkPtr<ElementType> &aEl = a[i];
                ElementType bEl = b[i];
                for (unsigned int j = computedCount; j < endCount; j++) {
                    dst[j] += aEl->getElement(j) * bEl;
                }
            }

            return endCount;
        };

        return new series::ParallelComputerSeries<ElementType, decltype(op), const program::ProgObjArray<series::DataSeries<ElementType> *>>(context, op, a);
    });

    resolver.decl("dot", [&context](const program::ProgObjArray<series::DataSeries<ElementType> *> a, const program::ProgObjArray<series::DataSeries<ElementType> *> b){
        auto op = [size = getSize(a, b)](ElementType *dst, unsigned int computedCount, const std::vector<series::ChunkPtr<ElementType>> &a, const std::vector<series::ChunkPtr<ElementType>> &b){
            assert(a.size() == size);
            assert(b.size() == size);

            unsigned int endCount = CHUNK_SIZE;
            for (const series::ChunkPtr<ElementType> &ptr : a) {
                unsigned int cc = ptr->getComputedCount();
                if (cc < endCount) {
                    endCount = cc;
                }
            }
            for (const series::ChunkPtr<ElementType> &ptr : b) {
                unsigned int cc = ptr->getComputedCount();
                if (cc < endCount) {
                    endCount = cc;
                }
            }

            if (endCount <= computedCount) {
                return computedCount;
            }

            for (unsigned int j = computedCount; j < endCount; j++) {
                dst[j] = 0.0;
            }

            for (std::size_t i = 0; i < size; i++) {
                const series::ChunkPtr<ElementType> &aEl = a[i];
                const series::ChunkPtr<ElementType> &bEl = b[i];
                for (unsigned int j = computedCount; j < endCount; j++) {
                    dst[j] += aEl->getElement(j) * bEl->getElement(j);
                }
            }

            return endCount;
        };

        return new series::ParallelComputerSeries<ElementType, decltype(op), const program::ProgObjArray<series::DataSeries<ElementType> *>, const program::ProgObjArray<series::DataSeries<ElementType> *>>(context, op, a, b);
    });
}

static int _ = program::Resolver::registerBuilder([](app::AppContext &context, program::Resolver &resolver) {
    declDotOp<float>(context, resolver);
    declDotOp<double>(context, resolver);
});
