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
#include <map>
#include <string>
#include <type_traits>
#include <typeindex>

namespace fetch {
namespace semanticsearch {

class ModelInstance : public std::enable_shared_from_this<ModelInstance>
{
public:
  using ModelInstancePtr = std::shared_ptr<ModelInstance>;
  using PropertyMap      = std::map<std::string, std::shared_ptr<ModelInstance>>;
  using PropertyVisitor  = std::function<void(std::string, ModelInstancePtr)>;

  /// Static pointer constructor
  /// @{
  template <typename T>
  static ModelInstancePtr New(T data);
  /// @}

  /// Only constructible by self
  /// @{
  ModelInstance()                           = delete;
  ModelInstance(ModelInstance const &other) = delete;
  ModelInstance &operator=(ModelInstance const &other) = delete;
  /// @}

  ~ModelInstance();

  /// Object manipulation
  /// @{
  void              Insert(std::string const &name, ModelInstancePtr const &value);
  ModelInstancePtr &operator[](std::string const &name);
  void              Walk(PropertyVisitor const &callback, std::string const &name = "");
  void              SetModelName(std::string const &name)
  {
    model_name_ = name;
  }

  /// @}

  /// Properteis
  /// @{
  std::type_index type() const;
  std::string     model_name() const
  {
    return model_name_;
  }
  int32_t expiry_block() const
  {
    return expiry_block_;
  }
  /// @}
private:
  /// Private constructor
  /// @{
  ModelInstance(std::type_index type, void *data);
  /// @}

  /// Data members
  /// @{
  std::type_index             type_;
  void *                      data_{nullptr};
  std::string                 model_name_;
  std::function<void(void *)> destructor_{nullptr};
  int32_t                     expiry_block_{0};
  /// @}

  /// Friends
  /// @{
  template <typename T>
  friend class TypedSchemaField;
  friend class ObjectSchemaField;
  /// @}
};

template <typename T>
ModelInstance::ModelInstancePtr ModelInstance::New(T data)
{
  ModelInstancePtr ret;
  ret.reset(new ModelInstance(std::type_index(typeid(T)), new T(data)));

  // Creating a destructor for the obejct of type T
  // since it is type erased and we don't know the type
  // when ModelInstance is destructed.
  ret->destructor_ = [](void *data) {
    T *d = static_cast<T *>(data);
    delete d;
  };

  return ret;
}

}  // namespace semanticsearch
}  // namespace fetch
