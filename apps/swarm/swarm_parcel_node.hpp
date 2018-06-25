#ifndef __SWARM_PARCEL_NODE_HPP
#define __SWARM_PARCEL_NODE_HPP

#include <iostream>
#include <string>
using std::cout;
using std::cerr;
using std::string;

namespace fetch
{
namespace swarm
{

class SwarmParcelNode
{
public:
  SwarmParcelNode(std::shared_ptr<SwarmNode> node, unsigned int protocolNumber):
    node_(node), protocolNumber_(protocolNumber)
  {
    node -> ToGetState([this](){
        LOCK_T mlock(mutex_);
        if (warehouse["block"].first.empty()) {
          return 0;
        }
        return int(warehouse["block"].first.front()[0]) & 0x0f;
      });
  }

  typedef std::shared_ptr<SwarmParcel> PARCEL_P;
  typedef std::string PARCEL_NAME;
  typedef std::string PARCEL_TYPE;

  virtual ~SwarmParcelNode()
  {
  }

  virtual std::string ClientNeedParcelList(const string &type, unsigned int count)
  {
    std::ostringstream ret;
    ret << "{" << endl;
    ret << "\"parcels\": [" << endl;

    int i = 0;
    for (auto &parcelName : ListParcelNames(type, count))
      {
        if (i)
          ret << "," << endl;
        i++;
        ret << "    \"" << parcelName << "\"";
      }
    if (i) {
      ret << endl;
    }
    ret << "  ]" << endl;
    ret << "}" << endl;
    return ret.str();
  }

  virtual std::string ClientNeedParcelData(const std::string &type, const std::string &parcelname)
  {
    std::ostringstream ret;

    if (HasParcel(type, parcelname))
      {
        auto data = GetParcel(type, parcelname) -> asJSON();
        ret << data << endl;
      }
     else
       {
         ret << "{}" << endl;
       }
    return ret.str();
  }

  void PublishParcel(const PARCEL_TYPE &type, const PARCEL_NAME &parcelname)
  {
    LOCK_T mlock(mutex_);
    if (HasParcel(type, parcelname))
      {
        auto iter = std::find(warehouse[type].first.begin(), warehouse[type].first.end(), parcelname);
        if (iter == warehouse[type].first.end())
          {
            warehouse[type].first.push_front(parcelname);
          }
      }
  }

  void StoreParcel(PARCEL_P parcel)
  {
    LOCK_T mlock(mutex_);
    auto name = parcel -> GetName();
    auto type = parcel -> GetType();
    warehouse[type].second[name] = parcel;
  }

  void DeleteParcel(const PARCEL_TYPE &type, const PARCEL_NAME &parcelname)
  {
    LOCK_T mlock(mutex_);
    warehouse[type].second.erase(parcelname);
    auto iter = std::find(warehouse[type].first.begin(), warehouse[type].first.end(), parcelname);
    if (iter == warehouse[type].first.end())
      {
        warehouse[type].first.erase(iter);
      }
  }

  void PublishParcel(PARCEL_P parcel)
  {
    // TODO(katie) prune the parcel lists.

    LOCK_T mlock(mutex_);

    auto type = parcel -> GetType();
    auto name = parcel -> GetName();

    StoreParcel(parcel);
    PublishParcel(type, name);
  }

  bool HasParcel(const PARCEL_TYPE &type, const PARCEL_NAME &parcelname)
  {
    auto citer = warehouse[type].second.find(parcelname);
    return citer != warehouse[type].second.end();
  }

  PARCEL_P GetParcel(const PARCEL_TYPE &type, const PARCEL_NAME &parcelname)
  {
    // TODO(katie) make this safe if not found
    LOCK_T mlock(mutex_);

    auto iter = warehouse[type].second.find(parcelname);
    return iter -> second;
  }

  std::list<std::string> ListParcelNames(const PARCEL_TYPE &type, unsigned int count)
  {
    // TODO(katie) prune the parcel lists.
    LOCK_T mlock(mutex_);

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
    std::shared_ptr<SwarmNode::clientType> client = node_ -> ConnectToPeer(peer);
    auto promise = client->Call(protocolNumber_, 1, type, count);
    promise.Wait();
    auto jsonResult = promise.As<std::string>();

    fetch::json::JSONDocument doc;
    doc.Parse(jsonResult);

    std::list<std::string> result;

    auto array = doc["parcels"];
    for(unsigned int i=0;i<array.size();i++)
      {
        std::string foo(array[i].as_byte_array());
        result.push_back(foo);
      }

    return result;
  }

  virtual std::string AskPeerForParcelData(const SwarmPeerLocation &peer, const string &type, const std::string &parcelid)
  {
    std::shared_ptr<SwarmNode::clientType> client = node_ -> ConnectToPeer(peer);
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
  typedef std::list<PARCEL_NAME>                          PARCEL_PUBLISH_LIST;
  typedef std::map<PARCEL_NAME, PARCEL_P>                 PARCELS_BY_NAME;
  typedef std::pair<PARCEL_PUBLISH_LIST, PARCELS_BY_NAME> PARCEL_STORAGE;
  typedef std::map<PARCEL_TYPE, PARCEL_STORAGE>           WAREHOUSE;

  WAREHOUSE                        warehouse;
  std::shared_ptr<SwarmNode>       node_;
  typedef std::recursive_mutex     MUTEX_T;
  typedef std::lock_guard<MUTEX_T> LOCK_T;
  mutable MUTEX_T                  mutex_;

  unsigned int protocolNumber_;
};

}
}

#endif //__SWARM_PARCEL_NODE_HPP
