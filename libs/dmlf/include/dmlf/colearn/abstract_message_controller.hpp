#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include <memory>

#include "core/byte_array/byte_array.hpp"
#include "dmlf/colearn/colearn_update.hpp"

namespace fetch {
namespace dmlf {
namespace colearn {

class AbstractMessageController
{
public:
  using Bytes          = ColearnUpdate::Data;
  using AlgorithmClass = ColearnUpdate::Algorithm;
  using UpdateClass    = ColearnUpdate::UpdateType;
  using UpdatePtr      = std::shared_ptr<ColearnUpdate>;
  using ConstUpdatePtr = std::shared_ptr<const ColearnUpdate>;

  AbstractMessageController()          = default;
  virtual ~AbstractMessageController() = default;

  // To implement
  virtual void           PushUpdate(UpdatePtr const &update, AlgorithmClass const &algorithm,
                                    UpdateClass const &upd_class)           = 0;
  virtual void           PushUpdate(Bytes const &update, AlgorithmClass const &algorithm,
                                    UpdateClass const &upd_class)           = 0;
  virtual std::size_t    GetUpdateCount(AlgorithmClass const &algorithm,
                                        UpdateClass const &   upd_class) const = 0;
  virtual std::size_t    GetUpdateTotalCount() const                        = 0;
  virtual ConstUpdatePtr GetUpdate(AlgorithmClass const &algorithm,
                                   UpdateClass const &   upd_class)            = 0;
};

}  // namespace colearn
}  // namespace dmlf
}  // namespace fetch
