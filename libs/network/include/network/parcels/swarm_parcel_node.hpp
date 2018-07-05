#ifndef SWARM_PARCEL_NODE_HPP
#define SWARM_PARCEL_NODE_HPP

#include <iostream>
#include <string>

#include "network/swarm/swarm_node.hpp"
#include "network/parcels/swarm_parcel.hpp"
#include "network/parcels/swarm_parcel_warehouse.hpp"
#include "network/interfaces/parcels/swarm_parcel_node_interface.hpp"
#include "network/protocols/parcels/commands.hpp"
#include "network/protocols/parcels/swarm_parcel_protocol.hpp"

namespace fetch
{
namespace swarm
{

  class SwarmParcelNode : public SwarmParcelNodeInterface, public SwarmParcelWarehouse
{
  std::shared_ptr<fetch::network::NetworkNodeCore> nnCore_;
public:
  SwarmParcelNode(std::shared_ptr<fetch::network::NetworkNodeCore> networkNodeCore)
    :
    nnCore_(networkNodeCore)
  {
    nnCore_ -> AddProtocol(this);
  }

  typedef fetch::network::NetworkNodeCore::client_type client_type;

  virtual ~SwarmParcelNode()
  {
  }

  virtual std::string ClientNeedParcelList(const std::string &type, unsigned int count)
  {
    std::cout << "ClientNeedParcelList!!!!" << std::endl;
    std::ostringstream ret;
    ret << "{" << std::endl;
    ret << "\"parcels\": [" << std::endl;

    /*int i = 0;
    for (auto &parcelName : ListParcelNames(type, count))
      {
        if (i)
          ret << "," << std::endl;
        i++;
        ret << "    \"" << parcelName << "\"";
      }
    if (i) {
      ret << std::endl;
      }*/
    ret << "  ]" << std::endl;
    ret << "}" << std::endl;
    std::cout << "ClientNeedParcelList!!!! done" << std::endl;
    std::cout << ret.str() << std::endl;
    return ret.str();
  }

  virtual std::string ClientNeedParcelData(const std::string &type, const std::string &parcelname)
  {
    std::ostringstream ret;

    if (HasParcel(type, parcelname))
      {
        auto data = GetParcel(type, parcelname) -> asJSON();
        ret << data << std::endl;
      }
     else
       {
         ret << "{}" << std::endl;
       }
    return ret.str();
  }

  virtual std::list<std::string> AskPeerForParcelIds(const SwarmPeerLocation &peer, const std::string &type, unsigned int count)
  {
    std::cout << "AskPeerForParcelIds " << peer.AsString() << " " << type << " " << count << " - conn" << std::endl;
    std::shared_ptr<client_type> client = nnCore_ -> ConnectToPeer(peer);
    std::cout << "AskPeerForParcelIds " << peer.AsString() << " " << type << " " << count << " - call pn=" << protocol_number << " cn=" << protocols::SwarmParcels::CLIENT_NEEDS_PARCEL_IDS << std::endl;
    auto promise = client->Call(protocol_number, protocols::SwarmParcels::CLIENT_NEEDS_PARCEL_IDS, type, count);
    std::cout << "AskPeerForParcelIds " << peer.AsString() << " " << type << " " << count << " - wait" << std::endl;
    promise.Wait(500);
    std::cout << "AskPeerForParcelIds " << peer.AsString() << " " << type << " " << count << " - yes!" <<  promise.is_fulfilled() << std::endl;

    if (!promise.is_fulfilled())
      {
        std::cout << "AskPeerForParcelIds " << peer.AsString() << " " << type << " " << count << " - Arg, failed." << std::endl;
        throw fetch::network::NetworkNodeCoreTimeOut("AskPeerForParcelIds");
      }

    auto jsonResult = promise.As<std::string>();
    std::cout << "AskPeerForParcelIds " << peer.AsString() << " " << type << " " << count << " - parse?" << std::endl;

    std::list<std::string> result;

    std::cout << "AskPeerForParcelIds " << peer.AsString() << " " << type << " " << count << " - 6" << std::endl;
    fetch::json::JSONDocument doc;
    std::cout << "AskPeerForParcelIds " << peer.AsString() << " " << type << " " << count << " - 7" << std::endl;
    try
      {
        doc.Parse(jsonResult);
        std::cout << "AskPeerForParcelIds " << peer.AsString() << " " << type << " " << count << " - parsed!" << std::endl;
      }
    catch(std::exception &x)
      {
        std::cout << "EXCEPTION PARSING JSON:" << x.what() << std::endl;
        return result;
      }

    std::cout << "AskPeerForParcelIds " << peer.AsString() << " " << type << " " << count << " - 9" << std::endl;
    auto array = doc["parcels"];
    std::cout << "AskPeerForParcelIds " << peer.AsString() << " " << type << " " << count << " - sz=" << array.size() << std::endl;
    for(unsigned int i=0;i<array.size();i++)
      {
        std::string foo(array[i].as_byte_array());
        result.push_back(foo);
      }
    return result;
  }

  virtual std::string AskPeerForParcelData(const SwarmPeerLocation &peer, const std::string &type, const std::string &parcelid)
  {
    std::shared_ptr<client_type> client = nnCore_ -> ConnectToPeer(peer);
    auto promise = client->Call(protocol_number, 2, type, parcelid);
    promise.Wait(500);
    if (!promise.is_fulfilled())
      {
        throw fetch::network::NetworkNodeCoreTimeOut("AskPeerForParcelData");
      }

    auto jsonResult = promise.As<std::string>();

    fetch::json::JSONDocument doc;

    try
      {
        doc.Parse(jsonResult);
      }
    catch(...)
      {
        return "";
      }
    doc.Parse(jsonResult);

    std::ostringstream ret;
    ret << doc["data"];
    std::string result(ret.str());

    return result;
  }

  SwarmParcelNode(const SwarmParcelNode &rhs)           = delete;
  SwarmParcelNode(SwarmParcelNode &&rhs)                = delete;
  SwarmParcelNode operator=(const SwarmParcelNode &rhs) = delete;
  SwarmParcelNode operator=(SwarmParcelNode &&rhs)      = delete;
  bool operator==(const SwarmParcelNode &rhs) const     = delete;
  bool operator<(const SwarmParcelNode &rhs) const      = delete;
protected:
  typedef std::recursive_mutex        mutex_type;
  typedef std::lock_guard<mutex_type> lock_type;
  mutable mutex_type                  mutex_;
};

}
}

#endif //SWARM_PARCEL_NODE_HPP
