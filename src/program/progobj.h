#pragma once

#include <variant>

#include "series/dataseries.h"
#include "series/finitecompseries.h"
#include "render/seriesrenderer.h"

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
    render::SeriesRenderer *
> ProgObj;

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

}
