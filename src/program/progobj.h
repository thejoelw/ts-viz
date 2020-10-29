#pragma once

#include <variant>
#include <string>
#include <cmath>
#include <vector>

#include "jw_util/hash.h"

namespace series { template <typename ElementType> class DataSeries; }
namespace series { template <typename ElementType> class FiniteCompSeries; }
namespace render { class SeriesRenderer; }

namespace program {

struct UncastNumber {
    UncastNumber()
        : value(std::nan(""))
    {}

    UncastNumber(double value)
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
    series::DataSeries<float> *,
    series::DataSeries<double> *,
    series::FiniteCompSeries<float> *,
    series::FiniteCompSeries<double> *,
    ProgObjArray<float>,
    ProgObjArray<double>,
    ProgObjArray<series::DataSeries<float> *>,
    ProgObjArray<series::DataSeries<double> *>,
    render::SeriesRenderer *
> ProgObj;

template <typename ItemType>
class ProgObjArray {
public:
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
    "uncast_number",
    "float",
    "double",
    "TS<float>",
    "TS<double>",
    "FS<float>",
    "FS<double>",
    "TS_renderer"
};

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
