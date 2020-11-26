#include "fftwx.h"

namespace series {

std::mutex fftwMutex;

/*
template<> bool FftwPlanner<float>::isInit = false;
template<> std::vector<const FftwPlanner<float>::IO> FftwPlanner<float>::ios;
template<> std::array<typename fftwx_impl<float>::Plan, CHUNK_SIZE_LOG2 + 1> FftwPlanner<float>::planFwds;
template<> std::array<typename fftwx_impl<float>::Plan, CHUNK_SIZE_LOG2 + 1> FftwPlanner<float>::planBwds;

template<> bool FftwPlanner<double>::isInit = false;
template<> std::vector<const FftwPlanner<double>::IO> FftwPlanner<double>::ios;
template<> std::array<typename fftwx_impl<double>::Plan, CHUNK_SIZE_LOG2 + 1> FftwPlanner<double>::planFwds;
template<> std::array<typename fftwx_impl<double>::Plan, CHUNK_SIZE_LOG2 + 1> FftwPlanner<double>::planBwds;
*/

}
