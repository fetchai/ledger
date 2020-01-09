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

#include "semanticsearch/query/query.hpp"
#include "semanticsearch/schema/vocabulary_instance.hpp"

namespace fetch {
namespace semanticsearch {

class ExecutionContext
{
public:
  using ConstByteArray = fetch::byte_array::ConstByteArray;
  using Vocabulary     = std::shared_ptr<VocabularyInstance>;

  Vocabulary Get(std::string const &name)
  {
    return context_[name];
  }

  void Set(std::string const &name, Vocabulary object, std::string type)
  {
    models_[name]  = std::move(type);
    context_[name] = std::move(object);
  }

  bool Has(std::string const &name)
  {
    return context_.find(name) != context_.end();
  }

  std::string GetModelName(std::string const &name)
  {
    return models_[name];
  }

private:
  std::map<std::string, Vocabulary>  context_;
  std::map<std::string, std::string> models_;
};

}  // namespace semanticsearch
}  // namespace fetch
