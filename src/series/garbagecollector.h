#pragma once

#include "log.h"
#include "app/tickercontext.h"
#include "app/options.h"
#include "jw_util/thread.h"

#include "defs/GARBAGE_COLLECTOR_LEVELS.h"

namespace series {

template <typename ObjectType>
class GarbageCollector : public app::TickerContext::TickableBase<GarbageCollector<ObjectType>> {
    static_assert(GARBAGE_COLLECTOR_LEVELS >= 1, "GARBAGE_COLLECTOR_LEVELS must be at least one!");

public:
    class Registration {
        friend class GarbageCollector;

    private:
        ObjectType *freeAfter = nullptr;
        ObjectType *freeBefore = nullptr;
    };

    struct Level {
        ObjectType *freeNext = nullptr;
    };

    GarbageCollector(app::AppContext &context)
        : app::TickerContext::TickableBase<GarbageCollector<ObjectType>>(context)
    {}

    void tick(app::TickerContext &tickerContext) {
        runGc();
    }

    void updateMemoryUsage(std::make_signed<std::size_t>::type inc) {
        jw_util::Thread::assert_main_thread();

        memoryUsage += inc;
        assert(static_cast<std::make_signed<std::size_t>::type>(memoryUsage) >= 0);

        if (inc > 0) {
            runGc();
        }
    }

    std::size_t getMemoryUsage() const {
        return memoryUsage;
    }

    void runGc() {
        jw_util::Thread::assert_main_thread();

        std::size_t memoryLimit = app::Options::getInstance().gcMemoryLimit;
        SPDLOG_DEBUG("Running GC; memory usage is {} / {}", memoryUsage, memoryLimit);
        while (memoryUsage > memoryLimit) {
            unsigned int i = 0;
            while (true) {
                ObjectType *next = levels[i].freeNext;
                if (next) {
                    delete next;
                    assert(levels[i].freeNext != next);
                    break;
                }

                if (++i == GARBAGE_COLLECTOR_LEVELS) {
                    return;
                }
            }
        }
    }

    void enqueue(ObjectType *obj) {
        jw_util::Thread::assert_main_thread();

        Level &level = getLevel(obj);
        Registration &reg = obj->getGcRegistration();
        assert(!reg.freeAfter == !reg.freeBefore);

        if (reg.freeAfter) {
            if (level.freeNext == obj) {
                if (obj->getGcRegistration().freeBefore == obj) {
                    return;
                } else {
                    level.freeNext = obj->getGcRegistration().freeBefore;
                }
            }

            // Remove from linked list
            reg.freeAfter->getGcRegistration().freeBefore = reg.freeBefore;
            reg.freeBefore->getGcRegistration().freeAfter = reg.freeAfter;
        }

        if (level.freeNext) {
            ObjectType *head = level.freeNext;
            ObjectType *tail = head->getGcRegistration().freeAfter;
            head->getGcRegistration().freeAfter = obj;
            tail->getGcRegistration().freeBefore = obj;
            obj->getGcRegistration().freeAfter = tail;
            obj->getGcRegistration().freeBefore = head;
        } else {
            level.freeNext = obj;
            obj->getGcRegistration().freeAfter = obj;
            obj->getGcRegistration().freeBefore = obj;
        }
    }

    void dequeue(ObjectType *obj) {
        jw_util::Thread::assert_main_thread();

        Level &level = getLevel(obj);
        Registration &reg = obj->getGcRegistration();
        assert(!reg.freeAfter == !reg.freeBefore);

        if (reg.freeAfter) {
            if (level.freeNext == obj) {
                if (obj->getGcRegistration().freeBefore == obj) {
                    level.freeNext = nullptr;
                } else {
                    level.freeNext = obj->getGcRegistration().freeBefore;
                }
            }

            // Remove from linked list
            reg.freeAfter->getGcRegistration().freeBefore = reg.freeBefore;
            reg.freeBefore->getGcRegistration().freeAfter = reg.freeAfter;

            reg.freeAfter = nullptr;
            reg.freeBefore = nullptr;
        }
    }

    void assertSequence(unsigned int levelIdx, const std::vector<ObjectType *> &seq) const {
        const Level &level = levels[levelIdx];
        if (seq.empty()) {
            assert(level.freeNext == nullptr);
        } else {
            assert(level.freeNext == seq.front());

            ObjectType *prev = seq.back();
            for (ObjectType *obj : seq) {
                assert(obj->getGcRegistration().freeAfter == prev);
                assert(prev->getGcRegistration().freeBefore == obj);
                prev = obj;
            }
        }
    }

private:
    std::size_t memoryUsage = 0;

    Level levels[GARBAGE_COLLECTOR_LEVELS];

    Level &getLevel(const ObjectType *obj) {
        if constexpr (GARBAGE_COLLECTOR_LEVELS == 1) {
            return levels[0];
        } else {
            std::uintptr_t x = reinterpret_cast<std::uintptr_t>(obj);
            x = (13 * x) ^ (x >> 15);
            return levels[x % GARBAGE_COLLECTOR_LEVELS];
        }
    }
};

}
