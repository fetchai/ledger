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

#include "dmlf/ilearner_networker.hpp"
#include <memory>

namespace fetch {
namespace dmlf {

class MuddleLearnerNetworker : public ILearnerNetworker
{
public:
  using Intermediate = byte_array::ByteArray;

  MuddleLearnerNetworker()
  {}
  virtual ~MuddleLearnerNetworker()
  {}

  virtual void        pushUpdate(std::shared_ptr<IUpdate> update);
  virtual std::size_t getUpdateCount() const;

protected:
  virtual Intermediate getUpdateIntermediate();

private:
  MuddleLearnerNetworker(const MuddleLearnerNetworker &other) = delete;
  MuddleLearnerNetworker &operator=(const MuddleLearnerNetworker &other)  = delete;
  bool                    operator==(const MuddleLearnerNetworker &other) = delete;
  bool                    operator<(const MuddleLearnerNetworker &other)  = delete;
};

}  // namespace dmlf
}  // namespace fetch
