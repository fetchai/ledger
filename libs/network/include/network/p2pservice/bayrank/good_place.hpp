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

#include "network/p2pservice/bayrank/trust_storage_interface.hpp"

#include <algorithm>
#include <unordered_map>
#include <vector>

namespace fetch {
namespace p2p {
namespace bayrank {

template <typename IDENTITY>
class GoodPlace : public TrustStorageInterface<IDENTITY>
{
protected:
  using TrustStorageInterface<IDENTITY>::storage_mutex_;
  using TrustStorageInterface<IDENTITY>::storage_;
  using TrustStorageInterface<IDENTITY>::id_store_;

public:
  using Trust                               = typename TrustStorageInterface<IDENTITY>::Trust;
  static constexpr char const *LOGGING_NAME = "GoodPlace";

  GoodPlace()  = default;
  ~GoodPlace() = default;

  GoodPlace(const GoodPlace &rhs) = delete;
  GoodPlace(GoodPlace &&rhs)      = delete;
  GoodPlace &operator=(const GoodPlace &rhs) = delete;
  GoodPlace &operator=(GoodPlace &&rhs) = delete;

  void AddPeer(Trust &&trust) override
  {
    FETCH_LOCK(storage_mutex_);
    if (this->IsInStoreLockLess(trust, "buffer"))
    {
      return;
    }
    auto const size = storage_.size();
    if (size >= MAX_SIZE)
    {
      if (storage_[size - 1].score <= trust.score)
      {
        id_store_[trust.peer_identity] = size - 1;
        id_store_.erase(storage_[size - 1].peer_identity);
        storage_[size - 1] = std::move(trust);
      }
    }
    else
    {
      id_store_[trust.peer_identity] = storage_.size();
      storage_.push_back(std::move(trust));
    }
    this->Sort();
  }

private:
  static constexpr std::size_t const MAX_SIZE = 1000;
};

}  // namespace bayrank
}  // namespace p2p
}  // namespace fetch