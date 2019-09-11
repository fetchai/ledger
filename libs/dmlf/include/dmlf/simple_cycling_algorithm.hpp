#pragma once

#include <memory>

#include "dmlf/ishuffle_algorithm.hpp"

namespace fetch {
namespace dmlf {

class SimpleCyclingAlgorithm: public IShuffleAlgorithm
{
public:
  SimpleCyclingAlgorithm(std::size_t count):IShuffleAlgorithm(count)
  {
    next = 0;
  }
  virtual ~SimpleCyclingAlgorithm()
  {
  }

  std::vector<std::size_t> getNextOutputs(std::size_t number_of_outputs);
protected:
private:
  std::size_t next;

  SimpleCyclingAlgorithm(const SimpleCyclingAlgorithm &other) = delete;
  SimpleCyclingAlgorithm &operator=(const SimpleCyclingAlgorithm &other) = delete;
  bool operator==(const SimpleCyclingAlgorithm &other) = delete;
  bool operator<(const SimpleCyclingAlgorithm &other) = delete;
};

}
}
