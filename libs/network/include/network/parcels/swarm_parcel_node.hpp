#ifndef SWARM_PARCEL_NODE_HPP
#define SWARM_PARCEL_NODE_HPP

#include <iostream>
#include <string>

#include "network/swarm/swarm_node.hpp"
#include "network/interfaces/parcels/swarm_parcel_node_interface.hpp"

namespace fetch
{
namespace swarm
{

class SwarmParcelNode : public SwarmParcelNodeInterface
{
  std::shared_ptr<fetch::network::NetworkNodeCore> nnCore_;
public:
  SwarmParcelNode(std::shared_ptr<fetch::network::NetworkNodeCore> networkNodeCore)
    :
    nnCore_(networkNodeCore)
  {
    nnCore_ -> AddProtocol(this);

    /*
      // TODO(katie) move this into top-level code.
      node -> ToGetState([this](){
      lock_type mlock(mutex_);
      if (warehouse["block"].first.empty()) {
      return 0;
      }
      return int(warehouse["block"].first.front()[0]) & 0x0f;
      });
    */
  }

  typedef fetch::network::NetworkNodeCore::client_type client_type;
  typedef std::shared_ptr<SwarmParcel> parcel_ptr;
  typedef std::string parcel_name_type;
  typedef std::string parcel_type_type;

  virtual ~SwarmParcelNode()
  {
  }

  virtual std::string ClientNeedParcelList(const string &type, unsigned int count)
  {
    std::cout << "ClientNeedParcelList!!!!" << std::endl;
    std::ostringstream ret;
    ret << "{" << std::endl;
    ret << "\"parcels\": [" << std::endl;

    int i = 0;
    for (auto &parcelName : ListParcelNames(type, count))
      {
        if (i)
          ret << "," << std::endl;
        i++;
        ret << "    \"" << parcelName << "\"";
      }
    if (i) {
      ret << std::endl;
    }
    ret << "  ]" << std::endl;
    ret << "}" << std::endl;
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

  void PublishParcel(const parcel_type_type &type, const parcel_name_type &parcelname)
  {
    lock_type mlock(mutex_);
    if (HasParcel(type, parcelname))
      {
        auto iter = std::find(warehouse[type].first.begin(), warehouse[type].first.end(), parcelname);
        if (iter == warehouse[type].first.end())
          {
            warehouse[type].first.push_front(parcelname);
          }
      }
  }

  void StoreParcel(parcel_ptr parcel)
  {
    lock_type mlock(mutex_);
    auto name = parcel -> GetName();
    auto type = parcel -> GetType();
    warehouse[type].second[name] = parcel;
  }

  void DeleteParcel(const parcel_type_type &type, const parcel_name_type &parcelname)
  {
    lock_type mlock(mutex_);
    warehouse[type].second.erase(parcelname);
    auto iter = std::find(warehouse[type].first.begin(), warehouse[type].first.end(), parcelname);
    if (iter == warehouse[type].first.end())
      {
        warehouse[type].first.erase(iter);
      }
  }

  void PublishParcel(parcel_ptr parcel)
  {
    // TODO(katie) prune the parcel lists.

    lock_type mlock(mutex_);

    auto type = parcel -> GetType();
    auto name = parcel -> GetName();

    StoreParcel(parcel);
    PublishParcel(type, name);
  }

  bool HasParcel(const parcel_type_type &type, const parcel_name_type &parcelname)
  {
    auto citer = warehouse[type].second.find(parcelname);
    return citer != warehouse[type].second.end();
  }

  parcel_ptr GetParcel(const parcel_type_type &type, const parcel_name_type &parcelname)
  {
    // TODO(katie) make this safe if not found
    lock_type mlock(mutex_);

    auto iter = warehouse[type].second.find(parcelname);
    return iter -> second;
  }

  std::list<std::string> ListParcelNames(const parcel_type_type &type, unsigned int count)
  {
    // TODO(katie) prune the parcel lists.
    lock_type mlock(mutex_);

    std::list<std::string> results;
    auto citer = warehouse[type].first.begin();
    while(citer != warehouse[type].first.end() && count > 0)
      {
        results.push_front(warehouse[type].second[*citer] -> GetName());
        ++citer;
        count--;
      }
    return results;
  }

  virtual std::list<std::string> AskPeerForParcelIds(const SwarmPeerLocation &peer, const string &type, unsigned int count)
  {
    std::shared_ptr<client_type> client = node_ -> ConnectToPeer(peer);
    auto promise = client->Call(protocolNumber_, 1, type, count);
    promise.Wait();
    auto jsonResult = promise.As<std::string>();

    //fetch::json::JSONDocument doc;
    //doc.Parse(jsonResult);

    std::list<std::string> result;

   // /auto array = doc["parcels"];
   // /for(unsigned int i=0;i<array.size();i++)
    //  {
    //    std::string foo(array[i].as_byte_array());
    //    result.push_back(foo);
    //  }

    return result;
  }

  virtual std::string AskPeerForParcelData(const SwarmPeerLocation &peer, const string &type, const std::string &parcelid)
  {
    std::shared_ptr<client_type> client = node_ -> ConnectToPeer(peer);
    auto promise = client->Call(protocolNumber_, 2, type, parcelid);
    promise.Wait();
    auto jsonResult = promise.As<std::string>();

    fetch::json::JSONDocument doc;
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
  typedef std::list<parcel_name_type>                                         published_parcels_list_type;
  typedef std::map<parcel_name_type, parcel_ptr>                              name_to_parcels_type;
  typedef std::pair<published_parcels_list_type, name_to_parcels_type>        parcel_storage_type;
  typedef std::map<parcel_type_type, parcel_storage_type>                     warehouse_type;

  warehouse_type                      warehouse;
  std::shared_ptr<SwarmNode>          node_;
  typedef std::recursive_mutex        mutex_type;
  typedef std::lock_guard<mutex_type> lock_type;
  mutable mutex_type                  mutex_;

  unsigned int protocolNumber_;
};

}
}

#endif //SWARM_PARCEL_NODE_HPP
