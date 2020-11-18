#pragma once

#include <string>

namespace app { class AppContext; }

namespace render {

class SeriesRenderer {
public:
    SeriesRenderer(app::AppContext &context, const std::string &name);

    virtual void draw(std::size_t begin, std::size_t end, std::size_t stride) = 0;

    const std::string &getName() const {
        return name;
    }

protected:
    app::AppContext &context;

    std::string name;
};

}
