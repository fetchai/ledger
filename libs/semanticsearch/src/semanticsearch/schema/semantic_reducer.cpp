#include "semanticsearch/schema/semantic_reducer.hpp"

namespace fetch {
namespace semanticsearch {

bool SemanticReducer::Validate(void *data)
{
  if (constraints_validation_)
  {
    return constraints_validation_(data);
  }

  return true;
}

SemanticPosition SemanticReducer::Reduce(void *data)
{
  if (reducer_)
  {
    return reducer_(data);
  }

  return {};
}

int SemanticReducer::rank() const
{

  return rank_;
}

}  // namespace semanticsearch
}  // namespace fetch