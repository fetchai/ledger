#include "network/p2pservice/p2p_resolver_protocol.hpp"
#include "network/p2pservice/p2p_resolver.hpp"
#include "network/p2pservice/p2p_service2.hpp"

namespace fetch {
namespace p2p {

ResolverProtocol::ResolverProtocol(Resolver &resolver, P2PService2 &p2p_service)
{
  Expose(QUERY, &resolver, &Resolver::Query);
  Expose(GET_MANIFEST, &p2p_service, &P2PService2::GetLocalManifest);
  Expose(GET_RANDOM_GOOD_PEERS, &p2p_service, &P2PService2::GetRandomGoodPeers);
  Expose(GET_NODE_URI, &p2p_service, &P2PService2::GetNodeUri);
}

}
}
