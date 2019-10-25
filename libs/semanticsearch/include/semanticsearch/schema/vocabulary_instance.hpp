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

#include <map>
#include <string>
#include <type_traits>
#include <typeindex>

namespace fetch {
namespace semanticsearch {

class VocabularyInstance : public std::enable_shared_from_this<VocabularyInstance>
{
public:
  using Vocabulary  = std::shared_ptr<VocabularyInstance>;
  using PropertyMap = std::map<std::string, std::shared_ptr<VocabularyInstance>>;

  template <typename T>
  static Vocabulary New(T data)
  {
    // TODO(tfr): add destructor
    return Vocabulary(new VocabularyInstance(std::type_index(typeid(T)), new T(data)));
  }

  VocabularyInstance()                                = delete;
  VocabularyInstance(VocabularyInstance const &other) = delete;
  VocabularyInstance &operator=(VocabularyInstance const &other) = delete;
  std::type_index     type() const
  {
    return type_;
  }

  void        Walk(std::function<void(std::string, Vocabulary)> callback, std::string name = "");
  Vocabulary &operator[](std::string name);
  void        Insert(std::string name, Vocabulary value);

private:
  VocabularyInstance(std::type_index type, void *data)
    : type_(std::move(type))
    , data_(std::move(data))
  {}

  std::type_index type_;
  void *          data_{nullptr};

  template <typename T>
  friend class DataToSubspaceMap;
  friend class PropertiesToSubspace;
};

}  // namespace semanticsearch
}  // namespace fetch
