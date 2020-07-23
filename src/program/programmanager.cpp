#include "programmanager.h"

#include "rapidjson/include/rapidjson/stringbuffer.h"
#include "rapidjson/include/rapidjson/writer.h"

#include "render/seriesrenderer.h"
#include "program/resolver.h"

namespace {

static std::string jsonToStr(const rapidjson::Value &value) {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    value.Accept(writer);
    return std::string(buffer.GetString(), buffer.GetSize());
}

}

namespace program {

ProgramManager::ProgramManager(app::AppContext &context)
    : context(context)
{}

void ProgramManager::recvRecord(const rapidjson::Document &row) {
    for (rapidjson::Value::ConstMemberIterator it = row.MemberBegin(); it != row.MemberEnd(); ++it) {
        ProgObj obj = makeProgObj(it->value);
        if (std::holds_alternative<series::Series<float> *>(obj)) {
            context.get<render::SeriesRenderer>().addSeries(it->name.GetString(), *std::get<series::Series<float> *>(obj));
//        } else if (std::holds_alternative<series::Series<double> *>(obj)) {
//            context.get<render::SeriesRenderer>().addSeries(it->name.GetString(), *std::get<series::Series<double> *>(obj));
        } else {
            throw InvalidProgramException("Value for top-level entry " + jsonToStr(it->name) + " must be a series");
        }
    }
}

ProgObj ProgramManager::makeProgObj(const rapidjson::Value &value) {
    switch (value.GetType()) {
        case rapidjson::Type::kNullType: return ProgObj();
        case rapidjson::Type::kFalseType: return ProgObj(false);
        case rapidjson::Type::kTrueType: return ProgObj(true);
        case rapidjson::Type::kNumberType: return ProgObj(UncastNumber(value.GetDouble()));
        case rapidjson::Type::kStringType: return ProgObj(std::string(value.GetString(), value.GetStringLength()));
        case rapidjson::Type::kObjectType: return ProgObj();
        case rapidjson::Type::kArrayType:
            const rapidjson::Value::ConstArray &arr = value.GetArray();
            if (arr.Size() < 1 || !arr[0].IsString()) {
                throw InvalidProgramException("Array must have length >= 1 and first item must be a string: " + jsonToStr(value));
            }
            std::string name(arr[0].GetString(), arr[0].GetStringLength());
            std::vector<ProgObj> args;
            for (std::size_t i = 1; i < arr.Size(); i++) {
                args.push_back(makeProgObj(arr[i]));
            }
            return context.get<Resolver>().call(name, args);
    }
}

}
