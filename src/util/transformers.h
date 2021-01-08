#pragma once

#include <utility>
#include <tuple>

namespace util {

template <typename RealType, typename... InnerTransformers>
struct TransformerGroup {
    TransformerGroup(InnerTransformers &&... innerTransformers)
        : innerTransformers(std::move(innerTransformers)...)
    {}

    TransformerGroup(const TransformerGroup &) = delete;
    TransformerGroup &operator=(const TransformerGroup &) = delete;

    void operator()(RealType *dst, const RealType *src) const {
        std::apply([dst, src](auto ... innerTransformers) {(innerTransformers(dst, src), ...);}, innerTransformers);
    }

    std::tuple<InnerTransformers...> innerTransformers;
};

template <typename RealType, std::size_t dstOffset, std::size_t srcOffset, typename InnerTransformer>
struct OffsetTransformer {
    OffsetTransformer(InnerTransformer &&innerTransformer)
        : innerTransformer(std::move(innerTransformer))
    {}

    OffsetTransformer(const OffsetTransformer &) = delete;
    OffsetTransformer &operator=(const OffsetTransformer &) = delete;

    void operator()(RealType *dst, const RealType *src) const {
        innerTransformer(dst + dstOffset, src + srcOffset);
    }

    InnerTransformer innerTransformer;
};

template <typename RealType, std::size_t size, typename InnerTransformer>
struct StaticTransformerLoop {
    StaticTransformerLoop(InnerTransformer &&innerTransformer)
        : innerTransformer(std::move(innerTransformer))
    {}

    StaticTransformerLoop(const StaticTransformerLoop &) = delete;
    StaticTransformerLoop &operator=(const StaticTransformerLoop &) = delete;

    void operator()(RealType *dst, const RealType *src) const {
        for (std::size_t i = 0; i < size; i++) {
            innerTransformer(dst++, src++);
        }
    }

    InnerTransformer innerTransformer;
};

template <typename RealType, typename InnerTransformer>
struct DynamicTransformerLoop {
    DynamicTransformerLoop(InnerTransformer &&innerTransformer, std::size_t size)
        : innerTransformer(std::move(innerTransformer))
        , size(size)
    {}

    DynamicTransformerLoop(const DynamicTransformerLoop &) = delete;
    DynamicTransformerLoop &operator=(const DynamicTransformerLoop &) = delete;

    void operator()(RealType *dst, const RealType *src) const {
        for (std::size_t i = 0; i < size; i++) {
            innerTransformer(dst++, src++);
        }
    }

    InnerTransformer innerTransformer;
    std::size_t size;
};

template <typename RealType, typename InnerTransformer>
struct OptionalTransformer {
    OptionalTransformer(InnerTransformer &&innerTransformer, bool enable)
        : innerTransformer(std::move(innerTransformer))
        , enable(enable)
    {}

    OptionalTransformer(const OptionalTransformer &) = delete;
    OptionalTransformer &operator=(const OptionalTransformer &) = delete;

    void operator()(RealType *dst, const RealType *src) const {
        if (enable) {
            innerTransformer(dst, src);
        }
    }

    InnerTransformer innerTransformer;
    bool enable;
};

template <typename RealType>
struct CopyTransformer {
    CopyTransformer(const CopyTransformer &) = delete;
    CopyTransformer &operator=(const CopyTransformer &) = delete;

    void operator()(RealType *dst, const RealType *src) const {
        *dst = *src;
    }
};

template <typename RealType>
struct IncrementTransformer {
    IncrementTransformer(const IncrementTransformer &) = delete;
    IncrementTransformer &operator=(const IncrementTransformer &) = delete;

    void operator()(RealType *dst, const RealType *src) const {
        *dst += *src;
    }
};

}
