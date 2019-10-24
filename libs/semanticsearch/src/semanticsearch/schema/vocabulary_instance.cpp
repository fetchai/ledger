#include "semanticsearch/schema/model_register.hpp"

namespace fetch {
namespace semanticsearch {

void VocabularyInstance::Walk(std::function<void(std::string, Vocabulary)> callback,
                              std::string                                  name)
{
  if (std::type_index(typeid(PropertyMap)) != type_)
  {
    callback(name, shared_from_this());
    return;
  }

  PropertyMap &map = *reinterpret_cast<PropertyMap *>(data_);
  for (auto &o : map)
  {
    o.second->Walk(callback, o.first);
  }
}

VocabularyInstance::Vocabulary &VocabularyInstance::operator[](std::string name)
{
  if (std::type_index(typeid(PropertyMap)) != type_)
  {
    throw std::runtime_error(
        "VocabularyInstance index operator error: Vocabulary does not have keys");
  }

  PropertyMap &map = *reinterpret_cast<PropertyMap *>(data_);

  return map[name];
}

void VocabularyInstance::Insert(std::string name, Vocabulary value)
{
  if (std::type_index(typeid(PropertyMap)) != type_)
  {
    throw std::runtime_error(
        "VocabularyInstance index operator error: Vocabulary does not have keys");
  }

  PropertyMap &map = *reinterpret_cast<PropertyMap *>(data_);
  map[name]        = value;
}

}  // namespace semanticsearch
}  // namespace fetch