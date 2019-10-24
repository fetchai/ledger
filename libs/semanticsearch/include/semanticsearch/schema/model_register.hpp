#pragma once
#include "semanticsearch/schema/properties_map.hpp"

#include <memory>
#include <unordered_map>

namespace fetch {
namespace semanticsearch {

class ModelRegister
{
public:
  using VocabularySchema    = std::shared_ptr<PropertiesToSubspace>;
  using SharedModelRegister = std::shared_ptr<ModelRegister>;

  ModelRegister()          = default;
  virtual ~ModelRegister() = default;

  void             AddModel(std::string name, VocabularySchema object);
  VocabularySchema GetModel(std::string name);
  bool             HasModel(std::string name);

  virtual void OnAddModel(std::string name, VocabularySchema object) = 0;

private:
  std::unordered_map<std::string, VocabularySchema> models_;
};

}  // namespace semanticsearch
}  // namespace fetch