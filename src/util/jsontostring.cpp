#include "jsontostring.h"

#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

namespace util {

std::string jsonToStr(const rapidjson::Value &value) {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    value.Accept(writer);
    return std::string(buffer.GetString(), buffer.GetSize());
}

}
