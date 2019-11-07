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

#include <functional>
#include <map>
#include <string>
#include <type_traits>
#include <typeindex>

namespace fetch {
namespace semanticsearch {

class VocabularyInstance : public std::enable_shared_from_this<VocabularyInstance>
{
public:
  using VocabularyInstancePtr = std::shared_ptr<VocabularyInstance>;
  using PropertyMap           = std::map<std::string, std::shared_ptr<VocabularyInstance>>;
  using PropertyVisitor       = std::function<void(std::string, VocabularyInstancePtr)>;

  /// Static pointer constructor
  /// @{
  template <typename T>
  static VocabularyInstancePtr New(T data);
  /// @}

  /// Only constructible by self
  /// @{
  VocabularyInstance()                                = delete;
  VocabularyInstance(VocabularyInstance const &other) = delete;
  VocabularyInstance &operator=(VocabularyInstance const &other) = delete;
  /// @}

  ~VocabularyInstance();

  /// Object manipulation
  /// @{
  void                   Insert(std::string const &name, VocabularyInstancePtr const &value);
  VocabularyInstancePtr &operator[](std::string const &name);
  void                   Walk(PropertyVisitor const &callback, std::string const &name = "");
  /// @}

  /// Properteis
  /// @{
  std::type_index type() const;
  /// @}
private:
  /// Private constructor
  /// @{
  VocabularyInstance(std::type_index type, void *data)
    : type_(type)
    , data_(data)
  {}
  /// @}

  /// Data members
  /// @{
  std::type_index             type_;
  void *                      data_{nullptr};
  std::function<void(void *)> destructor_{nullptr};
  /// @}

  /// Friends
  /// @{
  template <typename T>
  friend class VocabularyTypedField;
  friend class VocabularyObjectField;
  /// @}
};

template <typename T>
VocabularyInstance::VocabularyInstancePtr VocabularyInstance::New(T data)
{
  VocabularyInstancePtr ret;
  ret.reset(new VocabularyInstance(std::type_index(typeid(T)), new T(data)));
  ret->destructor_ = [](void *data) {
    T *d = static_cast<T *>(data);
    delete d;
  };

  return ret;
}

}  // namespace semanticsearch
}  // namespace fetch
