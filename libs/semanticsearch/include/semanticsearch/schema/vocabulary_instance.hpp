#pragma once

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
    // TODO: add destructor
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