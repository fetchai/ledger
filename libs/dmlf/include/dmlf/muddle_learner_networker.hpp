#pragma once

#include <memory>
#include "dmlf/ilearner_networker.hpp"

namespace fetch {
namespace dmlf {

class MuddleLearnerNetworker: public ILearnerNetworker
{
public:
  MuddleLearnerNetworker()
  {
  }
  virtual ~MuddleLearnerNetworker()
  {
  }

  virtual void pushUpdate( std::shared_ptr<IUpdate> update);
  virtual std::size_t getUpdateCount() const;
  virtual std::shared_ptr<IUpdate> getUpdate();
protected:
private:
  MuddleLearnerNetworker(const MuddleLearnerNetworker &other) = delete;
  MuddleLearnerNetworker &operator=(const MuddleLearnerNetworker &other) = delete;
  bool operator==(const MuddleLearnerNetworker &other) = delete;
  bool operator<(const MuddleLearnerNetworker &other) = delete;
};

}
}
