#include "semanticsearch/schema/model_register.hpp"

namespace fetch {
namespace semanticsearch {

void ModelRegister::AddModel(std::string name, VocabularySchema object)
{
  if (models_.find(name) != models_.end())
  {
    if (object->IsSame(models_[name]))
    {
      return;
    }

    throw std::runtime_error("Model '" + name + "' already exists, but definition mismatch.");
  }

  object->SetModelName(name);
  models_[name] = object;
  OnAddModel(name, object);
}

ModelRegister::VocabularySchema ModelRegister::GetModel(std::string name)
{
  if (models_.find(name) == models_.end())
  {
    return nullptr;
  }
  return models_[name];
}

bool ModelRegister::HasModel(std::string name)
{
  return models_.find(name) != models_.end();
}

}  // namespace semanticsearch
}  // namespace fetch