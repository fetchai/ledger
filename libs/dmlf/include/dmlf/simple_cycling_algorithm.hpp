#pragma once

#include <memory>

#include "dmlf/ishuffle_algorithm.hpp"

namespace fetch {
namespace dmlf {

class SimpleCyclingAlgorithm: public IShuffleAlgorithm
{
public:
  SimpleCyclingAlgorithm(std::size_t count, std::size_t number_of_outputs_per_cycle);
  virtual ~SimpleCyclingAlgorithm()
  {
  }

  std::vector<std::size_t> getNextOutputs();
protected:
private:
  std::size_t next_output_index;
  std::size_t number_of_outputs_per_cycle;

  SimpleCyclingAlgorithm(const SimpleCyclingAlgorithm &other) = delete;
  SimpleCyclingAlgorithm &operator=(const SimpleCyclingAlgorithm &other) = delete;
  bool operator==(const SimpleCyclingAlgorithm &other) = delete;
  bool operator<(const SimpleCyclingAlgorithm &other) = delete;
};

}
}
