#include "fftwx.h"

namespace series {

std::mutex fftwMutex;

template<> std::vector<FftwPlanIO<float>> FftwPlanIO<float>::planIOs;
template<> std::vector<FftwPlanIO<double>> FftwPlanIO<double>::planIOs;

}
