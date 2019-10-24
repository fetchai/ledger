#pragma once
#include "semanticsearch/index/base_types.hpp"
#include "semanticsearch/location.hpp"

#include <crypto/identity.hpp>
#include <memory>
#include <string>

namespace fetch {
namespace semanticsearch {

struct AgentProfile
{
  using Identity = fetch::crypto::Identity;
  using Agent    = std::shared_ptr<AgentProfile>;
  using AgentId  = uint64_t;

  static Agent New(AgentId id)
  {
    return Agent(new AgentProfile(id));
  }

  void RegisterLocation(std::string model, SemanticPosition position)
  {
    Location loc;

    loc.model    = std::move(model);
    loc.position = std::move(position);

    locations.insert(std::move(loc));
  }

  Identity           identity;
  AgentId            id;
  std::set<Location> locations;

private:
  AgentProfile(AgentId i)
    : id(std::move(i))
  {}
};

using Agent = AgentProfile::Agent;

}  // namespace semanticsearch
}  // namespace fetch