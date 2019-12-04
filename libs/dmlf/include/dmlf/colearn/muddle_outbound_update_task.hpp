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

#include "dmlf/deprecated/update_interface.hpp"
#include "muddle/muddle_interface.hpp"
#include "muddle/rpc/client.hpp"
#include "oef-base/threading/Task.hpp"

namespace fetch {
namespace dmlf {
namespace colearn {

class MuddleOutboundUpdateTask : public oef::base::Task
{
public:
  using MuddlePtr    = muddle::MuddlePtr;
  using RpcClient    = fetch::muddle::rpc::Client;
  using RpcClientPtr = std::shared_ptr<RpcClient>;
  using Bytes        = byte_array::ByteArray;
  using ExitState    = oef::base::ExitState;
  using Address      = fetch::muddle::Address;

  Address                      target_;
  std::string                  algo_name_;
  std::string                  type_name_;
  Bytes                        update_;
  RpcClientPtr                 client_;
  static constexpr char const *LOGGING_NAME = "MuddleOutboundUpdateTask";

  double proportion_;
  double random_factor_;

  MuddleOutboundUpdateTask(Address target, std::string algo_name, std::string type_name,
                           Bytes update, RpcClientPtr client, double proportion,
                           double random_factor)
    : target_(std::move(target))
    , algo_name_(std::move(algo_name))
    , type_name_(std::move(type_name))
    , update_(std::move(update))
    , client_(std::move(client))
    , proportion_(proportion)
    , random_factor_(random_factor)
  {}
  ~MuddleOutboundUpdateTask() override = default;

  ExitState run() override;
  bool      IsRunnable() const override;

  MuddleOutboundUpdateTask(MuddleOutboundUpdateTask const &other) = delete;
  MuddleOutboundUpdateTask &operator=(MuddleOutboundUpdateTask const &other)  = delete;
  bool                      operator==(MuddleOutboundUpdateTask const &other) = delete;
  bool                      operator<(MuddleOutboundUpdateTask const &other)  = delete;

protected:
private:
};

}  // namespace colearn
}  // namespace dmlf
}  // namespace fetch
