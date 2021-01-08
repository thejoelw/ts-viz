#pragma once

#include <string>

#include "rapidjson/document.h"

namespace util {

std::string jsonToStr(const rapidjson::Value &value);

}
