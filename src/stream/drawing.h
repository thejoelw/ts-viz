#pragma once

#include <vector>

namespace stream {

class Drawing {
public:
    Drawing()
        : lastPt(NAN, NAN)
    {}

    void reset() {
        lastPt = glm::vec2(NAN, NAN);
        pts.clear();
    }

    void addPoint(glm::vec2 pt) {
        if (std::isnan(lastPt.x)) {
            lastPt = pt;
        }

        std::vector<glm::vec2>::iterator begin = std::upper_bound(pts.begin(), pts.end(), lastPt, Comparator());
        std::vector<glm::vec2>::iterator end = std::upper_bound(pts.begin(), pts.end(), pt, Comparator());
        if (begin == end) {
            pts.insert(end, pt);
        } else {
            if (end < begin) {
                std::swap(begin, end);
            }
            *begin++ = pt;
            pts.erase(begin, end);
        }

        lastPt = pt;
    }

    double sample(double x) {
        std::vector<glm::vec2>::iterator it = std::lower_bound(pts.begin(), pts.end(), glm::vec2(x, 0.0f), Comparator());
        if (it->x == x) {
            return it->y;
        } else if (it == pts.begin()) {
            return NAN;
        } else if (it == pts.end()) {
            return NAN;
        } else {
            glm::dvec2 b = *it;
            glm::dvec2 a = *--it;
            double t = (x - a.x) / (b.x - a.x);
            return a.y * (1.0 - t) + b.y * t;
        }
    }

private:
    struct Comparator {
        bool operator() (const glm::vec2& lhs, const glm::vec2& rhs) const {
            return lhs.x < rhs.x;
        }
    };

    glm::vec2 lastPt;
    std::vector<glm::vec2> pts;
};

}
