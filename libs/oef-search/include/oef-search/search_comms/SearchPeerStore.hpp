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

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>

class SearchPeerStore
{
public:
  static constexpr char const *LOGGING_NAME = "SearchPeerStore";

  using Mutex   = std::mutex;
  using Lock    = std::lock_guard<Mutex>;
  using ForBody = std::function<void(const std::string &)>;

  SearchPeerStore()                             = default;
  virtual ~SearchPeerStore()                    = default;
  SearchPeerStore(const SearchPeerStore &other) = delete;
  SearchPeerStore &operator=(const SearchPeerStore &other) = delete;

  bool operator==(const SearchPeerStore &other) = delete;
  bool operator<(const SearchPeerStore &other)  = delete;

  void AddPeer(const std::string &peer)
  {
    Lock lock(mutex_);
    store_.insert(peer);
  }

  void ForAllPeer(ForBody const &func)
  {
    Lock lock(mutex_);
    for (const auto &peer : store_)
    {
      func(peer);
    }
  }

protected:
private:
  Mutex                           mutex_;
  std::unordered_set<std::string> store_;
};
