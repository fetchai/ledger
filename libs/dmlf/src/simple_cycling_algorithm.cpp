#include "dmlf/simple_cycling_algorithm.hpp"

namespace fetch {
namespace dmlf {

std::vector<std::size_t> SimpleCyclingAlgorithm::getNextOutputs()
{
  std::vector<std::size_t> result(number_of_outputs_per_cycle);
  for(std::size_t i = 0; i< number_of_outputs_per_cycle; i++)
  {
    result[i] = next_output_index;
    next_output_index += 1;
    next_output_index %= getCount();
  }
  return result;
}

SimpleCyclingAlgorithm::SimpleCyclingAlgorithm(std::size_t count, std::size_t number_of_outputs_per_cycle):IShuffleAlgorithm(count)
{
  this -> next_output_index = 0;
  this -> number_of_outputs_per_cycle = std::min(number_of_outputs_per_cycle, getCount());
}

}
}
