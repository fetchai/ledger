#pragma once

#include <memory>
#include "dmlf/ilearner_networker.hpp"

namespace fetch {
namespace dmlf {

class LocalLearnerNetworker: public ILearnerNetworker
{
public:
  LocalLearnerNetworker();
  virtual ~LocalLearnerNetworker();
  virtual void pushUpdate( std::shared_ptr<IUpdate> update);
  virtual std::size_t getUpdateCount() const;
  virtual std::shared_ptr<IUpdate> getUpdate();

  virtual std::size_t getCount() = 0;
protected:
private:
  std::size_t ident;

  static std::size_t &getCounter();

  LocalLearnerNetworker(const LocalLearnerNetworker &other) = delete;
  LocalLearnerNetworker &operator=(const LocalLearnerNetworker &other) = delete;
  bool operator==(const LocalLearnerNetworker &other) = delete;
  bool operator<(const LocalLearnerNetworker &other) = delete;
};

}
}
