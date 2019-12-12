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

#include "dmlf/colearn/colearn_update.hpp"

#include "core/serializers/main_serializer.hpp"
#include "crypto/hash.hpp"
#include "crypto/sha256.hpp"

namespace fetch {
namespace dmlf {
namespace colearn {

ColearnUpdate::ColearnUpdate(Algorithm algorithm, UpdateType update_type, Data &&data,
                             Source source, Metadata metadata)
  : algorithm_(std::move(algorithm))
  , update_type_{std::move(update_type)}
  , data_{std::move(data)}
  , source_{std::move(source)}
  , metadata_{std::move(metadata)}
  , creation_{Clock::now()}
  , fingerprint_{ComputeFingerprint()}
{}

ColearnUpdate::Resolution ColearnUpdate::TimeSinceCreation() const
{
  return std::chrono::duration_cast<Resolution>(Clock::now() - creation_);
}

ColearnUpdate::Fingerprint ColearnUpdate::ComputeFingerprint()
{
  serializers::LargeObjectSerializeHelper serializer;
  serializer << algorithm();
  serializer << update_type();
  serializer << source();
  serializer << data();
  return crypto::Hash<crypto::SHA256>(serializer.data());
}

}  // namespace colearn
}  // namespace dmlf
}  // namespace fetch
