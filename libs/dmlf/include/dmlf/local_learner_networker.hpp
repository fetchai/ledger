#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

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

  virtual std::size_t getCount();

  static void resetAll(void);
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
