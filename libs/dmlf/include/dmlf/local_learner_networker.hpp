#pragma once

#include <memory>
#include "dmlf/ilearner_networker.hpp"

namespace fetch {
namespace dmlf {

class LocalLearnerNetworker: public ILearnerNetworker
{
public:
  LocalLearnerNetworker()
  {
  }
  virtual ~LocalLearnerNetworker()
  {
  }

  virtual void pushUpdate( std::shared_ptr<IUpdate> update) = 0;
  virtual std::size_t getUpdateCount() const = 0;
  virtual std::shared_ptr<IUpdate> getUpdate() = 0;

protected:
private:
  LocalLearnerNetworker(const LocalLearnerNetworker &other) = delete;
  LocalLearnerNetworker &operator=(const LocalLearnerNetworker &other) = delete;
  bool operator==(const LocalLearnerNetworker &other) = delete;
  bool operator<(const LocalLearnerNetworker &other) = delete;
};

}
}
