#pragma once

#include <vector>

namespace app { class AppContext; }

namespace series {

template <typename ElementType>
class FiniteCompSeries {
public:
    FiniteCompSeries(app::AppContext &context, std::vector<ElementType> &&data)
        : data(std::move(data))
    {
        (void) context;
    }

    std::size_t getSize() const {
        return data.size();
    }

    const ElementType *getData() const {
        return data.data();
    }

private:
    std::vector<ElementType> data;
};

}
