#pragma once

#include <memory>

#include "dmlf/ishuffle_algorithm.hpp"

namespace fetch {
namespace dmlf {

class IUpdate;

class ILearnerNetworker
{
public:
  ILearnerNetworker()
  {
  }
  virtual ~ILearnerNetworker()
  {
  }

  virtual void pushUpdate( std::shared_ptr<IUpdate> update) = 0;
  virtual std::size_t getUpdateCount() const = 0;
  virtual std::shared_ptr<IUpdate> getUpdate() = 0;

  virtual void setShuffleAlgorithm(std::shared_ptr<IShuffleAlgorithm> alg)
  {
    this -> alg = alg;
  }

  virtual std::size_t getCount() = 0;
protected:
private:
  std::shared_ptr<IShuffleAlgorithm> alg;

  ILearnerNetworker(const ILearnerNetworker &other) = delete;
  ILearnerNetworker &operator=(const ILearnerNetworker &other) = delete;
  bool operator==(const ILearnerNetworker &other) = delete;
  bool operator<(const ILearnerNetworker &other) = delete;
};

}
}
