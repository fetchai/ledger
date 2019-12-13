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

#include "semanticsearch/schema/semantic_reducer.hpp"

#include <cassert>

namespace fetch {
namespace semanticsearch {

SemanticReducer::SemanticReducer(std::string const &uid)
  : unique_identifier_{uid}
{}

bool SemanticReducer::Validate(void const *data, std::string &error) const
{
  if (constraints_validation_)
  {
    return constraints_validation_(data, error);
  }

  return true;
}

SemanticPosition SemanticReducer::Reduce(void const *data) const
{
  if (reducer_)
  {
    return reducer_(data);
  }

  return {};
}

bool SemanticReducer::operator==(SemanticReducer const &other) const
{
  return unique_identifier_ == other.unique_identifier_;
}

bool SemanticReducer::operator<=(SemanticReducer const &other) const
{
  return unique_identifier_ <= other.unique_identifier_;
}

bool SemanticReducer::operator<(SemanticReducer const &other) const
{
  return unique_identifier_ < other.unique_identifier_;
}

bool SemanticReducer::operator>=(SemanticReducer const &other) const
{
  return unique_identifier_ >= other.unique_identifier_;
}

bool SemanticReducer::operator>(SemanticReducer const &other) const
{
  return unique_identifier_ > other.unique_identifier_;
}

int SemanticReducer::rank() const
{

  return rank_;
}

}  // namespace semanticsearch
}  // namespace fetch
