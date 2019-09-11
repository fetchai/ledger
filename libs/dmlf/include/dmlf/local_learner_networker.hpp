#pragma once

#include <map>
#include <memory>
#include <list>
#include "dmlf/ilearner_networker.hpp"
#include "core/mutex.hpp"

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
  using LocalLearnerNetworkerIndex = std::map<std::size_t, LocalLearnerNetworker*>;
  using Mutex = fetch::Mutex;
  using Lock = std::unique_lock<Mutex>;
  using IUpdateP = std::shared_ptr<IUpdate>;
  using UpdateList = std::list<IUpdateP>;

  std::size_t ident;

  static std::size_t &getCounter();
  static Mutex index_mutex;
  static LocalLearnerNetworkerIndex index;

  UpdateList updates;
  mutable Mutex mutex;

  LocalLearnerNetworker(const LocalLearnerNetworker &other) = delete;
  LocalLearnerNetworker &operator=(const LocalLearnerNetworker &other) = delete;
  bool operator==(const LocalLearnerNetworker &other) = delete;
  bool operator<(const LocalLearnerNetworker &other) = delete;
};

}
}
