#pragma once

#include <memory>

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

protected:
private:
  ILearnerNetworker(const ILearnerNetworker &other) = delete;
  ILearnerNetworker &operator=(const ILearnerNetworker &other) = delete;
  bool operator==(const ILearnerNetworker &other) = delete;
  bool operator<(const ILearnerNetworker &other) = delete;
};

}
}
