#include "semanticsearch/agent_directory.hpp"

namespace fetch {
namespace semanticsearch {

AgentDirectory::AgentId AgentDirectory::RegisterAgent(ConstByteArray const &pk)
{
  if (pk_to_id_.find(pk) != pk_to_id_.end())
  {
    throw std::runtime_error("Agent already registered.");
  }

  AgentId id    = counter_;
  agents_[id]   = AgentProfile::New(id);
  pk_to_id_[pk] = id;

  ++counter_;
  return id;
}

AgentDirectory::Agent AgentDirectory::GetAgent(ConstByteArray const &pk)
{
  if (pk_to_id_.find(pk) == pk_to_id_.end())
  {
    return nullptr;
  }
  auto id = pk_to_id_[pk];
  return agents_[id];
}

void AgentDirectory::UnregisterAgent(AgentId /*id*/)
{
  throw std::runtime_error("Unregister not implemented yet.");
}

bool AgentDirectory::RegisterLocation(AgentId id, std::string model, SemanticPosition vector)
{
  if (agents_.find(id) == agents_.end())
  {
    return false;
  }

  agents_[id]->RegisterLocation(std::move(model), std::move(vector));

  return true;
}

}  // namespace semanticsearch
}  // namespace fetch
