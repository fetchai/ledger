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

#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "core/serializers/main_serializer.hpp"
#include "crypto/hash.hpp"
#include "crypto/sha256.hpp"
#include "ledger/dag/dag_epoch.hpp"

#include <set>

namespace fetch {
namespace ledger {

bool DAGEpoch::Contains(DAGHash const &digest) const
{
  return all_nodes.find(digest) != all_nodes.end();
}

void DAGEpoch::Finalise()
{
  // strictly speaking this is a bit of a weird hash because it will also contain all the weird
  // serialisation metadata
  serializers::MsgPackSerializer buf;
  buf << *this;

  this->hash.type = DAGHash::Type::EPOCH;
  this->hash.hash = crypto::Hash<crypto::SHA256>(buf.data());
}

}  // namespace ledger
}  // namespace fetch
