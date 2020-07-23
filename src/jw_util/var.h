#ifndef JWUTIL_VAR_H
#define JWUTIL_VAR_H

#include "jw_util/signal.h"

namespace jw_util {

template <typename DataType, typename SignalType = const DataType &>
class Var {
private:
    class ChangeSignal : public jw_util::SignalEmitter<SignalType> {
        friend class Var<DataType, SignalType>;
    };

public:
    typedef typename jw_util::SignalEmitter<SignalType>::Listener ChangeListener;

    Var(const DataType &initData)
        : data(initData)
    {}

    const DataType &get() const {
        return data;
    }

    void set(const DataType &newData) {
        if (data != newData) {
            data = newData;
            onChange.trigger(data);
        }
    }

    mutable ChangeSignal onChange;

private:
    DataType data;
};

}

#endif // JWUTIL_VAR_H
