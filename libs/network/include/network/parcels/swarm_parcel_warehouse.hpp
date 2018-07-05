#ifndef SWARM_PARCEL_WAREHOUSE_HPP
#define SWARM_PARCEL_WAREHOUSE_HPP

#include <iostream>
#include <string>

#include "network/swarm/swarm_node.hpp"
#include "network/parcels/swarm_parcel.hpp"
#include "network/interfaces/parcels/swarm_parcel_node_interface.hpp"
#include "network/protocols/parcels/commands.hpp"

namespace fetch
{
namespace swarm
{

class SwarmParcelWarehouse
{
public:
  SwarmParcelWarehouse()
  {
  }

  typedef std::shared_ptr<SwarmParcel> parcel_ptr;
  typedef std::string parcel_name_type;
  typedef std::string parcel_type_type;

  virtual ~SwarmParcelWarehouse()
  {
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

  SwarmParcelWarehouse(const SwarmParcelWarehouse &rhs)           = delete;
  SwarmParcelWarehouse(SwarmParcelWarehouse &&rhs)                = delete;
  SwarmParcelWarehouse operator=(const SwarmParcelWarehouse &rhs) = delete;
  SwarmParcelWarehouse operator=(SwarmParcelWarehouse &&rhs)      = delete;
  bool operator==(const SwarmParcelWarehouse &rhs) const     = delete;
  bool operator<(const SwarmParcelWarehouse &rhs) const      = delete;
protected:
  typedef std::list<parcel_name_type>                                         published_parcels_list_type;
  typedef std::map<parcel_name_type, parcel_ptr>                              name_to_parcels_type;
  typedef std::pair<published_parcels_list_type, name_to_parcels_type>        parcel_storage_type;
  typedef std::map<parcel_type_type, parcel_storage_type>                     warehouse_type;

  warehouse_type                      warehouse;
  typedef std::recursive_mutex        mutex_type;
  typedef std::lock_guard<mutex_type> lock_type;
  mutable mutex_type                  mutex_;
};

}
}

#endif //SWARM_PARCEL_WAREHOUSE_HPP
