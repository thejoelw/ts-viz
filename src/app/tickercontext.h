#pragma once

#include "jw_util/context.h"
#include "jw_util/contextbuilder.h"

#include "app/appcontext.h"

namespace app {

class TickerContext : public jw_util::Context<TickerContext> {
public:
    template <typename ClassType>
    class TickCaller {
    public:
        TickCaller(TickerContext &tickerContext) {
            tickerContext.getAppContext().template get<ClassType>().tick(tickerContext);
        }
    };

    template <typename ClassType>
    class ScopedCaller {
    public:
        ScopedCaller(TickerContext &tickerContext)
            : tickerContext(tickerContext)
        {
            tickerContext.getAppContext().template get<ClassType>().tickOpen(tickerContext);
        }

        ~ScopedCaller() {
            tickerContext.getAppContext().template get<ClassType>().tickClose(tickerContext);
        }

    private:
        TickerContext &tickerContext;
    };

    template <typename DerivedType, typename TickerType = TickCaller<DerivedType>>
    class TickableBase {
    public:
        typedef TickerType Ticker;

        TickableBase(AppContext &context)
            : context(context)
        {
            context.get<TickerContext>().template insert<TickerType>();
        }

        ~TickableBase() {
            context.get<TickerContext>().template remove<TickerType>();
        }

    protected:
        app::AppContext &context;
    };


    TickerContext(AppContext &appContext)
        : Context(0.1f)
        , appContext(appContext)
    {}

    ~TickerContext() {
        assert(builder.getSize() == 0);
    }

    AppContext &getAppContext() {
        return appContext;
    }

    template <typename TickerType>
    void insert() {
        builder.registerConstructor<TickerType>();
    }

    template <typename TickerType>
    void remove() {
        builder.removeConstructor<TickerType>();
    }

    void tick();

    void log(LogLevel level, const std::string &msg);

private:
    AppContext &appContext;
    jw_util::ContextBuilder<TickerContext> builder;
};

}
