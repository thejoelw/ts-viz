#pragma once

#include <ratio>
#include <vector>
#include <deque>

#include "graphics/glvao.h"
#include "graphics/glbuffer.h"
#include "jw_util/signal.h"

namespace graphics {

template <typename SharedType, typename LocalType, typename allocRatio = std::ratio<3, 2>>
class SplitBuffer {
public:
    struct Viewer {
        Viewer(unsigned int index, const SharedType &shared, const LocalType &local)
            : index(index)
            , shared(shared)
            , local(local)
        {}

        const unsigned int index;
        const SharedType &shared;
        const LocalType &local;
    };
    struct Reader {
        Reader(unsigned int index, const SharedType &shared, LocalType &local)
            : index(index)
            , shared(shared)
            , local(local)
        {}

        const unsigned int index;
        const SharedType &shared;
        LocalType &local;

        operator Viewer() {
            return Viewer(index, shared, local);
        }
    };
    struct Mutator {
        Mutator(unsigned int index, SharedType &shared, LocalType &local)
            : index(index)
            , shared(shared)
            , local(local)
        {}

        const unsigned int index;
        SharedType &shared;
        LocalType &local;

        operator Reader() {
            return Reader(index, shared, local);
        }
        operator Viewer() {
            return Viewer(index, shared, local);
        }
    };

    SplitBuffer(GLenum target, GLenum bufferHint)
        : remoteBuffer(target, bufferHint)
    {}

    std::size_t getExtentSize() const {
        return sharedData.size();
    }

    std::size_t getActiveSize() const {
        return sharedData.size() - availableIndices.size();
    }

    const std::vector<unsigned int> &getAvailableIndices() const {
        return availableIndices;
    }

    GlBufferBase &getGlBuffer() {
        return remoteBuffer;
    }

    Mutator create() {
        unsigned int index;
        if (availableIndices.empty()) {
            index = sharedData.size();
            sharedData.emplace_back();
            localData.emplace_back();
        } else {
            index = availableIndices.back();
            availableIndices.pop_back();

            new(&sharedData[index]) SharedType;
            new(&localData[index]) LocalType;
        }

        return mutate(index);
    }

    void destroy(unsigned int index) {
        assert(index != static_cast<unsigned int>(-1));

        sharedData[index].~SharedType();
        localData[index].~LocalType();

        availableIndices.push_back(index);
        //observer->onFree(index);
    }

    Viewer view(unsigned int index) const {
        assert(index < sharedData.size());
        return Viewer(index, sharedData[index], localData[index]);
    }
    Reader read(unsigned int index) {
        assert(index < sharedData.size());
        return Reader(index, sharedData[index], localData[index]);
    }
    Mutator mutate(unsigned int index) {
        assert(index < sharedData.size());
        remoteBuffer.flag_index(index);
        return Mutator(index, sharedData[index], localData[index]);
    }

    void setupVao(GlVao &vao) {
        vao.assertBound();
        remoteBuffer.bind();
        SharedType::setupVao(vao);
    }

    void sync(GlVao &vao) {
        remoteBuffer.bind();
        if (remoteBuffer.needs_resize(getExtentSize())) {
            remoteBuffer.update_size(getExtentSize());
            setupVao(vao);
            onNewVbo.trigger(*this);
        }
        remoteBuffer.update_flags(sharedData.cbegin());
    }

    jw_util::Signal<SplitBuffer<SharedType, LocalType, allocRatio> &> onNewVbo;

private:
    // TODO: Make into a chunked vector so we can use glBufferSubData
    std::deque<SharedType> sharedData;
    std::deque<LocalType> localData;
    std::vector<unsigned int> availableIndices;

    GlBuffer<SharedType, allocRatio> remoteBuffer;
};

}
