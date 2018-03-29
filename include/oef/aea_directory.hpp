#ifndef AEA_DIRECTORY_HPP
#define AEA_DIRECTORY_HPP

#include"protocols/node_to_aea/commands.hpp"

// This file manages connections to AEAs that have registered for callbacks

namespace fetch
{
namespace oef
{

class AEADirectory {
  public:
    void Register(uint64_t client, std::string id) {
      std::cout << "\rRegistering " << client << " with id " << id << std::endl << std::endl << "> " << std::flush;

      mutex_.lock();
      registered_aeas_[client] = id; // TODO: (`HUT`) : proper management of this
      mutex_.unlock();
    }

    void Deregister(uint64_t client, std::string id) {
      std::cout << "\rDeregistering " << client << " with id " << id << std::endl << std::endl << "> " << std::flush;
      std::lock_guard< fetch::mutex::Mutex > lock(mutex_);

      registered_aeas_[client] = ""; // TODO: (`HUT`) : proper management of this (check corresponding id)
    }

    void PingAllAEAs() {
      std::lock_guard< fetch::mutex::Mutex > lock(mutex_);

      detailed_assert( service_ != nullptr);

      for(auto &id: registered_aeas_) {
        auto &rpc = service_->ServiceInterfaceOf(id.first);

        rpc.Call(protocols::FetchProtocols::NODE_TO_AEA, protocols::NodeToAEAReverseRPC::PING, "ping_message"); // TODO: (`HUT`) : think about decoupling
      }
    }

    script::Variant BuyFromAEA(const std::string &buyer, const fetch::byte_array::BasicByteArray &id) {
      script::Variant result = script::Variant::Object();
      std::lock_guard< fetch::mutex::Mutex > lock(mutex_);

      std::string aeaID{id};

      for (std::map<uint32_t,std::string>::const_iterator it=registered_aeas_.begin(); it!=registered_aeas_.end(); ++it){
        if(aeaID.compare(it->second) == 0){
          result["response"] = "success";

          auto &rpc = service_->ServiceInterfaceOf(it->first);
          std::string answer   = rpc.Call(protocols::FetchProtocols::NODE_TO_AEA, protocols::NodeToAEAReverseRPC::BUY, buyer).As<std::string>();
          result["value"]   = answer;
          return result;
        }
      }

      result["response"] = "fail";
      std::string build{"AEA id: '"};
      build += id;
      build += "' not active";
      result["reason"]   = build;

      return result;
    }

    void register_service_instance( service::ServiceServer< fetch::network::TCPServer > *ptr)
    {
      service_ = ptr;
    }

  private:
    service::ServiceServer< fetch::network::TCPServer > *service_ = nullptr;
    std::map< uint32_t, std::string >                    registered_aeas_;
    fetch::mutex::Mutex                                  mutex_;
};

}
}

#endif
