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

#include "dmlf/colearn/colearn_protocol.hpp"
#include "dmlf/colearn/muddle_learner_networker_impl.hpp"

namespace fetch {
namespace dmlf {
namespace colearn {

ColearnProtocol::ColearnProtocol(MuddleLearnerNetworkerImpl &exec)
{
  ExposeWithClientContext(RPC_COLEARN_UPDATE, &exec,
                          &MuddleLearnerNetworkerImpl::NetworkColearnUpdate);
}

}  // namespace colearn
}  // namespace dmlf
}  // namespace fetch
