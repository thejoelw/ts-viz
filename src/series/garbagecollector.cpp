#include "garbagecollector.h"

#include "util/testrunner.h"
#include "log.h"

class Obj {
public:
    Obj(app::AppContext &context, unsigned int &extantMask, unsigned int myMask)
        : context(context)
        , extantMask(extantMask)
        , myMask(myMask)
    {
        assert((extantMask & myMask) == 0);
        extantMask |= myMask;

        context.get<series::GarbageCollector<Obj>>().updateMemoryUsage(1);
    }

    ~Obj() {
        assert((extantMask & myMask) == myMask);
        extantMask &= ~myMask;

        context.get<series::GarbageCollector<Obj>>().updateMemoryUsage(-1);
        context.get<series::GarbageCollector<Obj>>().dequeue(this);
    }

    series::GarbageCollector<Obj>::Registration &getGcRegistration() {
        return gcReg;
    }

private:
    app::AppContext &context;

    series::GarbageCollector<Obj>::Registration gcReg;

    unsigned int &extantMask;
    unsigned int myMask;
};

static int _ = util::TestRunner::getInstance().registerTest([](app::AppContext &context) {
    if constexpr (GARBAGE_COLLECTOR_LEVELS == 1) {
        std::size_t origGcMemoryLimit = app::Options::getMutableInstance().gcMemoryLimit;
        app::Options::getMutableInstance().gcMemoryLimit = 10;

        unsigned int extantMask = 0;
        Obj *obj0 = new Obj(context, extantMask, 1 << 0);
        Obj *obj1 = new Obj(context, extantMask, 1 << 1);
        Obj *obj2 = new Obj(context, extantMask, 1 << 2);
        Obj *obj3 = new Obj(context, extantMask, 1 << 3);

        SPDLOG_DEBUG("Check empty GC");
        assert(context.get<series::GarbageCollector<Obj>>().getMemoryUsage() == 4);
        assert(extantMask == 0b1111);
        context.get<series::GarbageCollector<Obj>>().assertSequence(0, {});

        SPDLOG_DEBUG("Check enqueue 0->1 works");
        context.get<series::GarbageCollector<Obj>>().enqueue(obj0);
        context.get<series::GarbageCollector<Obj>>().assertSequence(0, {obj0});

        SPDLOG_DEBUG("Check enqueue 1->1 works");
        context.get<series::GarbageCollector<Obj>>().enqueue(obj0);
        context.get<series::GarbageCollector<Obj>>().assertSequence(0, {obj0});

        SPDLOG_DEBUG("Check dequeue 1->1 works");
        context.get<series::GarbageCollector<Obj>>().dequeue(obj1);
        context.get<series::GarbageCollector<Obj>>().assertSequence(0, {obj0});

        SPDLOG_DEBUG("Check dequeue 1->0 works");
        context.get<series::GarbageCollector<Obj>>().dequeue(obj0);
        context.get<series::GarbageCollector<Obj>>().assertSequence(0, {});

        SPDLOG_DEBUG("Check dequeue 0->0 works");
        context.get<series::GarbageCollector<Obj>>().dequeue(obj0);
        context.get<series::GarbageCollector<Obj>>().assertSequence(0, {});

        SPDLOG_DEBUG("Check enqueue 0->2 works");
        context.get<series::GarbageCollector<Obj>>().enqueue(obj0);
        context.get<series::GarbageCollector<Obj>>().enqueue(obj1);
        context.get<series::GarbageCollector<Obj>>().assertSequence(0, {obj0, obj1});

        SPDLOG_DEBUG("Check enqueue 2->2 (without movement) works");
        context.get<series::GarbageCollector<Obj>>().enqueue(obj1);
        context.get<series::GarbageCollector<Obj>>().assertSequence(0, {obj0, obj1});

        SPDLOG_DEBUG("Check enqueue 2->2 (with movement) works");
        context.get<series::GarbageCollector<Obj>>().enqueue(obj0);
        context.get<series::GarbageCollector<Obj>>().assertSequence(0, {obj1, obj0});

        SPDLOG_DEBUG("Check dequeue 2->2 works");
        context.get<series::GarbageCollector<Obj>>().dequeue(obj2);
        context.get<series::GarbageCollector<Obj>>().assertSequence(0, {obj1, obj0});

        SPDLOG_DEBUG("Check dequeue 2->1 (at tail) works");
        context.get<series::GarbageCollector<Obj>>().dequeue(obj0);
        context.get<series::GarbageCollector<Obj>>().assertSequence(0, {obj1});

        SPDLOG_DEBUG("Check dequeue 2->1 (at head) works");
        context.get<series::GarbageCollector<Obj>>().enqueue(obj0);
        context.get<series::GarbageCollector<Obj>>().dequeue(obj1);
        context.get<series::GarbageCollector<Obj>>().assertSequence(0, {obj0});

        SPDLOG_DEBUG("Check dequeue 3->2 (in the middle) works");
        context.get<series::GarbageCollector<Obj>>().enqueue(obj2);
        context.get<series::GarbageCollector<Obj>>().enqueue(obj1);
        context.get<series::GarbageCollector<Obj>>().dequeue(obj2);
        context.get<series::GarbageCollector<Obj>>().assertSequence(0, {obj0, obj1});

        SPDLOG_DEBUG("Check enqueue 2->4 works");
        context.get<series::GarbageCollector<Obj>>().enqueue(obj2);
        context.get<series::GarbageCollector<Obj>>().enqueue(obj3);
        context.get<series::GarbageCollector<Obj>>().assertSequence(0, {obj0, obj1, obj2, obj3});

        SPDLOG_DEBUG("Check memory at limit is fine");
        app::Options::getMutableInstance().gcMemoryLimit = 4;
        context.get<series::GarbageCollector<Obj>>().runGc();
        assert(context.get<series::GarbageCollector<Obj>>().getMemoryUsage() == 4);
        assert(extantMask == 0b1111);
        context.get<series::GarbageCollector<Obj>>().assertSequence(0, {obj0, obj1, obj2, obj3});

        SPDLOG_DEBUG("Check removing one item works");
        app::Options::getMutableInstance().gcMemoryLimit = 3;
        context.get<series::GarbageCollector<Obj>>().runGc();
        assert(context.get<series::GarbageCollector<Obj>>().getMemoryUsage() == 3);
        assert(extantMask == 0b1110);
        context.get<series::GarbageCollector<Obj>>().assertSequence(0, {obj1, obj2, obj3});

        SPDLOG_DEBUG("Check removing two items");
        app::Options::getMutableInstance().gcMemoryLimit = 1;
        context.get<series::GarbageCollector<Obj>>().runGc();
        assert(context.get<series::GarbageCollector<Obj>>().getMemoryUsage() == 1);
        assert(extantMask == 0b1000);
        context.get<series::GarbageCollector<Obj>>().assertSequence(0, {obj3});

        SPDLOG_DEBUG("Check removing last item");
        app::Options::getMutableInstance().gcMemoryLimit = 0;
        context.get<series::GarbageCollector<Obj>>().runGc();
        assert(context.get<series::GarbageCollector<Obj>>().getMemoryUsage() == 0);
        assert(extantMask == 0b0000);
        context.get<series::GarbageCollector<Obj>>().assertSequence(0, {});

        app::Options::getMutableInstance().gcMemoryLimit = origGcMemoryLimit;
    } else {
        SPDLOG_WARN("Skipping GarbageCollector test because GARBAGE_COLLECTOR_LEVELS is not 1, making things non-deterministic");
    }
});
