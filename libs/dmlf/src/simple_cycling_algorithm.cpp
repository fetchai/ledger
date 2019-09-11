#include "dmlf/simple_cycling_algorithm.hpp"

namespace fetch {
namespace dmlf {

std::vector<std::size_t> SimpleCyclingAlgorithm::getNextOutputs(std::size_t number_of_outputs)
{
  number_of_outputs = std::min(number_of_outputs, getCount());
  std::vector<std::size_t> result(number_of_outputs);
  for(std::size_t i = 0; i< number_of_outputs; i++)
  {
    result[i] = next;
    next += 1;
    next %= getCount();
  }
  return result;
}

}
}
