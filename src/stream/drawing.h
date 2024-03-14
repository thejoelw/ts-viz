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

        bool isFwd = pt.x > lastPt.x;
        std::vector<glm::vec2>::iterator begin = std::lower_bound(pts.begin(), pts.end(), isFwd ? lastPt : pt, Comparator());
        std::vector<glm::vec2>::iterator end = std::upper_bound(pts.begin(), pts.end(), isFwd ? pt : lastPt, Comparator());
        if (begin == end) {
            pts.insert(end, pt);
        } else {
            if (isFwd) {
                if (begin->x == lastPt.x) { begin++; }
            } else {
                if (std::prev(end)->x == lastPt.x) { end--; }
            }

            if (begin == end) {
                pts.insert(end, pt);
            } else {
                *begin++ = pt;
                pts.erase(begin, end);
            }
        }

        lastPt = pt;
    }

    double sample(double x) const {
        std::vector<glm::vec2>::const_iterator it = std::lower_bound(pts.cbegin(), pts.cend(), glm::vec2(x, 0.0f), Comparator());
        if (it == pts.cend()) {
            return NAN;
        } else if (it == pts.cbegin()) {
            return NAN;
        } else if (it->x == x) {
            return it->y;
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
