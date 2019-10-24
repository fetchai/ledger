#pragma once

#include "semanticsearch/query/query.hpp"
#include "semanticsearch/schema/vocabulary_instance.hpp"

namespace fetch {
namespace semanticsearch {

class ExecutionContext
{
public:
  using ConstByteArray = fetch::byte_array::ConstByteArray;
  using Vocabulary     = std::shared_ptr<VocabularyInstance>;

  Vocabulary Get(std::string name)
  {
    return context_[std::move(name)];
  }

  void Set(std::string name, Vocabulary object, std::string type)
  {
    models_[name]             = std::move(type);
    context_[std::move(name)] = std::move(object);
  }

  bool Has(std::string name)
  {
    return context_.find(name) != context_.end();
  }

  std::string GetModelName(std::string name)
  {
    return models_[name];
  }

private:
  std::map<std::string, Vocabulary>  context_;
  std::map<std::string, std::string> models_;
};

}  // namespace semanticsearch
}  // namespace fetch