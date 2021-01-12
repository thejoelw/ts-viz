#pragma once

#include <variant>
#include <string>
#include <cmath>
#include <vector>

#include "jw_util/hash.h"

#include "series/dataseries.decl.h"

namespace render { class SeriesRenderer; }
namespace stream { class SeriesEmitter; }

namespace program {

class UncastNumber {
public:
    UncastNumber(double value = NAN)
        : value(value)
    {}

    double value;

    bool operator==(const UncastNumber other) const {
        return value == other.value;
    }
};

template <typename ItemType>
class ProgObjArray;

typedef std::variant<
    std::monostate,
    std::string,
    bool,
    UncastNumber,
    float,
    double,
    std::int64_t,
    series::DataSeries<float> *,
    series::DataSeries<double> *,
    ProgObjArray<float>,
    ProgObjArray<double>,
    ProgObjArray<series::DataSeries<float> *>,
    ProgObjArray<series::DataSeries<double> *>,
    render::SeriesRenderer *,
    stream::SeriesEmitter *
> ProgObj;

template <typename ItemType>
class ProgObjArray {
public:
    ProgObjArray() {}
    ProgObjArray(std::vector<ItemType> &&arr)
        : arr(std::move(arr))
    {}

    const std::vector<ItemType> &getArr() const {
        return arr;
    }

    bool operator==(const ProgObjArray<ItemType> &other) const {
        return arr == other.arr;
    }

private:
    std::vector<ItemType> arr;
};

static std::string progObjTypeNames[] = {
    "null",
    "string",
    "bool",
    "UncastNumber",
    "float",
    "double",
    "int64",
    "TS<float>",
    "TS<double>",
    "Array<float>",
    "Array<double>",
    "Array<TS<float>>",
    "Array<TS<double>>",
    "TS_renderer",
    "TS_emitter"
};

static_assert(std::variant_size<ProgObj>::value == sizeof(progObjTypeNames) / sizeof(std::string), "ProgObj size doesn't match progObjTypeNames size");

}

namespace std {

template <>
struct hash<program::UncastNumber> {
    size_t operator()(const program::UncastNumber num) const {
        return std::hash<double>{}(num.value);
    }
};

template <typename ItemType>
struct hash<program::ProgObjArray<ItemType>> {
    size_t operator()(const program::ProgObjArray<ItemType> &arr) const {
        std::size_t hash = 0;
        for (const ItemType &item : arr.getArr()) {
            hash = jw_util::Hash<std::size_t>::combine(hash, std::hash<ItemType>{}(item));
        }
        return hash;
    }
};

}
