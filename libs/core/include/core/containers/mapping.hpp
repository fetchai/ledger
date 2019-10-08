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

#include <type_traits>
#include <unordered_map>

namespace fetch {
namespace core {

template <typename Key, typename Value>
class Mapping
{
public:
  // Construction / Destruction
  Mapping()                    = default;
  Mapping(Mapping const &)     = default;
  Mapping(Mapping &&) noexcept = default;
  ~Mapping()                   = default;

  bool Lookup(Key const &key, Value &value);
  bool Lookup(Value const &value, Key &key);
  void Update(Key const &key, Value const &value);

  // Operators
  Mapping &operator=(Mapping const &) = default;
  Mapping &operator=(Mapping &&) noexcept = default;

private:
  using ForwardIndex = std::unordered_map<Key, Value>;
  using ReverseIndex = std::unordered_map<Value, Key>;

  ForwardIndex forward_index_;
  ReverseIndex reverse_index_;

  static_assert(!std::is_same<Key, Value>::value, "");
};

template <typename K, typename V>
bool Mapping<K, V>::Lookup(K const &key, V &value)
{
  bool success{false};

  auto const it = forward_index_.find(key);
  if (it != forward_index_.end())
  {
    value   = it->second;
    success = true;
  }

  return success;
}

template <typename K, typename V>
bool Mapping<K, V>::Lookup(V const &value, K &key)
{
  bool success{false};

  auto const it = reverse_index_.find(value);
  if (it != reverse_index_.end())
  {
    key     = it->second;
    success = true;
  }

  return success;
}

template <typename K, typename V>
void Mapping<K, V>::Update(K const &key, V const &value)
{
  // update the forward map
  auto const forward_it = forward_index_.find(key);
  if (forward_it == forward_index_.end())
  {
    forward_index_.emplace(key, value);
  }
  else
  {
    forward_it->second = value;
  }

  // update the reverse map
  auto const reverse_it = reverse_index_.find(value);
  if (reverse_it == reverse_index_.end())
  {
    reverse_index_.emplace(value, key);
  }
  else
  {
    reverse_it->second = key;
  }
}

}  // namespace core
}  // namespace fetch
