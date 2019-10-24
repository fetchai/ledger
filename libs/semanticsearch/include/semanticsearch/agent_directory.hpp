#pragma once
#include "semanticsearch/agent_profile.hpp"
#include "semanticsearch/index/base_types.hpp"

#include <map>

namespace fetch {
namespace semanticsearch {

class AgentDirectory
{
public:
  using AgentId        = AgentProfile::AgentId;
  using ConstByteArray = fetch::byte_array::ConstByteArray;
  using Agent          = AgentProfile::Agent;

  AgentId RegisterAgent(ConstByteArray const &pk);
  Agent   GetAgent(ConstByteArray const &pk);
  void    UnregisterAgent(AgentId id);
  bool    RegisterLocation(AgentId id, std::string model, SemanticPosition vector);

private:
  AgentId                                     counter_{0};
  std::unordered_map<ConstByteArray, AgentId> pk_to_id_;
  std::map<AgentId, Agent>                    agents_;
};

}  // namespace semanticsearch
}  // namespace fetch