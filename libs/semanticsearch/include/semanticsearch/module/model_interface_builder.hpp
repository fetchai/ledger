#pragma once

#include "semanticsearch/schema/properties_map.hpp"
#include "semanticsearch/schema/subspace_map_interface.hpp"

#include <memory>

namespace fetch {
namespace semanticsearch {

class SematicSearchModule;

class ModelIntefaceBuilder
{
public:
  using ModelField       = std::shared_ptr<VocabularyToSubspaceMapInterface>;
  using VocabularySchema = std::shared_ptr<PropertiesToSubspace>;

  ModelIntefaceBuilder(VocabularySchema model = nullptr, SematicSearchModule *factory = nullptr);
                        operator bool() const;
  ModelIntefaceBuilder &Field(std::string name, std::string type);
  ModelIntefaceBuilder &Field(std::string name, ModelIntefaceBuilder proxy);
  ModelIntefaceBuilder &Field(std::string name, ModelField model);

  ModelIntefaceBuilder Vocabulary(std::string name);

  VocabularySchema model()
  {
    return model_;
  }

private:
  VocabularySchema     model_;
  SematicSearchModule *factory_;
};

}  // namespace semanticsearch
}  // namespace fetch