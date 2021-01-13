#pragma once

#include "jw_util/context.h"
#include "jw_util/contextbuilder.h"

#include "app/appcontext.h"

#include "defs/PRINT_TICK_ORDER.h"
#if PRINT_TICK_ORDER
#include "log.h"
#endif

namespace app {

class TickerContext : public jw_util::Context<TickerContext> {
public:
    template <typename ClassType>
    class TickCaller {
    public:
        TickCaller(TickerContext &tickerContext) {
#if PRINT_TICK_ORDER
            SPDLOG_TRACE("> Tick order: Enter {}.tick()", jw_util::TypeName::get<ClassType>());
#endif
            tickerContext.getAppContext().template get<ClassType>().tick(tickerContext);
#if PRINT_TICK_ORDER
            SPDLOG_TRACE("> Tick order: Exit {}.tick()", jw_util::TypeName::get<ClassType>());
#endif
        }
    };

    template <typename ClassType>
    class ScopedCaller {
    public:
        ScopedCaller(TickerContext &tickerContext)
            : tickerContext(tickerContext)
        {
#if PRINT_TICK_ORDER
            SPDLOG_TRACE("> Tick order: Enter {}.tickOpen()", jw_util::TypeName::get<ClassType>());
#endif
            tickerContext.getAppContext().template get<ClassType>().tickOpen(tickerContext);
#if PRINT_TICK_ORDER
            SPDLOG_TRACE("> Tick order: Exit {}.tickOpen()", jw_util::TypeName::get<ClassType>());
#endif
        }

        ~ScopedCaller() {
#if PRINT_TICK_ORDER
            SPDLOG_TRACE("> Tick order: Enter {}.tickClose()", jw_util::TypeName::get<ClassType>());
#endif
            tickerContext.getAppContext().template get<ClassType>().tickClose(tickerContext);
#if PRINT_TICK_ORDER
            SPDLOG_TRACE("> Tick order: Exit  {}.tickClose()", jw_util::TypeName::get<ClassType>());
#endif
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
