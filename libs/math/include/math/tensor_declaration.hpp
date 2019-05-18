#pragma once

#include "vectorise/memory/array.hpp"
#include "vectorise/memory/shared_array.hpp"

namespace fetch
{
namespace math
{

template <typename DataType, typename ContainerType = memory::SharedArray<DataType>>
class Tensor;

}
}