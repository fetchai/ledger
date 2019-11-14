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

#include <functional>
#include <memory>

#include "dmlf/colearn/colearn_update.hpp"

namespace fetch {
namespace dmlf {
namespace colearn {

class UpdateStoreInterface
{
public:
  using Update    = ColearnUpdate;
  using UpdatePtr = std::shared_ptr<const Update>;
  using Score     = double;
  using Criteria  = std::function<Score(UpdatePtr)>;

  using Algorithm  = Update::Algorithm;
  using UpdateType = Update::UpdateType;
  using Data       = Update::Data;
  using Source     = Update::Source;
  using Metadata   = Update::Metadata;
  using Consumer   = std::string;

  UpdateStoreInterface()                                  = default;
  virtual ~UpdateStoreInterface()                         = default;
  UpdateStoreInterface(UpdateStoreInterface const &other) = delete;
  UpdateStoreInterface &operator=(UpdateStoreInterface const &other) = delete;

  virtual void      PushUpdate(Algorithm const &algo, UpdateType update, Data &&data, Source source,
                               Metadata &&metadata)                                         = 0;
  virtual UpdatePtr GetUpdate(Algorithm const &algo, UpdateType const &type, Criteria criteria,
                              Consumer consumer = "")                                       = 0;
  virtual UpdatePtr GetUpdate(Algorithm const &algo, UpdateType const &type, Consumer = "") = 0;
  virtual std::size_t GetUpdateCount() const                                                = 0;
  virtual std::size_t GetUpdateCount(Algorithm const &algo, UpdateType const &type) const   = 0;
  // TODO(Juan) Set maximum timestamp? Queue size? Other such admin tasks
};

}  // namespace colearn
}  // namespace dmlf
}  // namespace fetch
