#pragma once

#include <memory>
#include <vector>

namespace fetch {
namespace dmlf {

class IShuffleAlgorithm
{
public:
  IShuffleAlgorithm(std::size_t count)
  {
    this -> count = count;
  }
  virtual ~IShuffleAlgorithm()
  {
  }

  virtual std::vector<std::size_t> getNextOutputs(std::size_t number_of_outputs) = 0;

  std::size_t getCount() const { return count; }
protected:
private:
  IShuffleAlgorithm(const IShuffleAlgorithm &other) = delete;
  IShuffleAlgorithm &operator=(const IShuffleAlgorithm &other) = delete;
  bool operator==(const IShuffleAlgorithm &other) = delete;
  bool operator<(const IShuffleAlgorithm &other) = delete;

  std::size_t count;
};

}
}
