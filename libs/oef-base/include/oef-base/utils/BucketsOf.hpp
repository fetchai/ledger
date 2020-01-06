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

#include <map>
#include <mutex>
#include <string>
#include <vector>

template <class CONTENTS, class NAME_TYPE = std::string, class IDENT_TYPE = std::size_t,
          std::size_t BUCKET_SIZE = 128>
class BucketsOf
{
public:
  using Mutex = std::mutex;
  using Lock  = std::lock_guard<Mutex>;

  BucketsOf()          = default;
  virtual ~BucketsOf() = default;

  IDENT_TYPE get(const NAME_TYPE &name)
  {
    Lock lock(mutex);

    auto iter = names.find(name);
    if (iter == names.end())
    {
      auto x      = lockless_extend();
      names[name] = x;
      return x;
    }
    return iter->second;
  }

  bool has(const NAME_TYPE &name) const
  {
    Lock lock(mutex);

    auto iter = names.find(name);
    return (iter != names.end());
  }

  IDENT_TYPE lockless_extend()
  {
    if (size >= BUCKET_SIZE * buckets.size())
    {
      auto newBucket = new Bucket(BUCKET_SIZE);
      buckets.push_back(newBucket);
    }
    auto r = size++;
    return IDENT_TYPE(r);
  }

  std::vector<std::pair<NAME_TYPE, IDENT_TYPE>> GetNames()
  {
    std::vector<std::pair<NAME_TYPE, IDENT_TYPE>> result;
    Lock                                          lock(mutex);
    for (const auto &name2id : names)
    {
      result.push_back(name2id);
    }
    return result;
  }

  CONTENTS &access(std::size_t index)
  {
    return (*(buckets[index / BUCKET_SIZE]))[index % BUCKET_SIZE];
  }
  const CONTENTS &access(std::size_t index) const
  {
    return (*(buckets[index / BUCKET_SIZE]))[index % BUCKET_SIZE];
  }

protected:
  using Bucket      = std::vector<CONTENTS>;
  using BucketStore = std::vector<Bucket *>;
  BucketStore                     buckets;
  mutable Mutex                   mutex;
  std::map<NAME_TYPE, IDENT_TYPE> names;
  std::size_t                     size = 0;

private:
  BucketsOf(const BucketsOf &other) = delete;
  BucketsOf &operator=(const BucketsOf &other)  = delete;
  bool       operator==(const BucketsOf &other) = delete;
  bool       operator<(const BucketsOf &other)  = delete;
};
