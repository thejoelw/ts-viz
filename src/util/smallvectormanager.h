#pragma once

#include <vector>

namespace app { class AppContext; }

namespace util {

// TODO: Make this actually do something interesting

template <typename DataType>
class SmallVectorManager {
public:
    class Ref {
    public:
        Ref() {}
        ~Ref() {
            assert(vec.capacity() == 0);
        }

        void alloc(SmallVectorManager<DataType> &manager) {
            (void) manager;
        }

        void release(SmallVectorManager<DataType> &manager) {
            (void) manager;
            vec.clear();
            vec.shrink_to_fit();
        }

        const DataType *data(const SmallVectorManager<DataType> &manager) const {
            (void) manager;
            return vec.data();
        }
        DataType *data(SmallVectorManager<DataType> &manager) {
            (void) manager;
            return vec.data();
        }

        bool empty() const {
            return vec.empty();
        }
        void clear(SmallVectorManager<DataType> &manager) {
            (void) manager;
            vec.clear();
        }

        std::size_t size() const {
            return vec.size();
        }
        void resize(SmallVectorManager<DataType> &manager, std::size_t newSize) {
            (void) manager;
            vec.resize(newSize);
        }

        DataType &back(SmallVectorManager<DataType> &manager) {
            (void) manager;
            return vec.back();
        }
        const DataType &back(SmallVectorManager<DataType> &manager) const {
            (void) manager;
            return vec.back();
        }
        template <typename T>
        void push_back(SmallVectorManager<DataType> &manager, T el) {
            (void) manager;
            vec.push_back(std::forward<T>(el));
        }
        void pop_back(SmallVectorManager<DataType> &manager) {
            (void) manager;
            vec.pop_back();
        }

        template <typename T>
        void remove(SmallVectorManager<DataType> &manager, T el) {
            (void) manager;
            typename std::vector<DataType>::iterator found = std::find(vec.begin(), vec.end(), el);
            assert(found != vec.end());
            *found = vec.back();
            vec.pop_back();
        }

    private:
        std::vector<DataType> vec;
    };

    SmallVectorManager(app::AppContext &context) {
        (void) context;
    }
};

}
