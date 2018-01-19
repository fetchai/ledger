#ifndef RPC_FEED_HPP
#define RPC_FEED_HPP
#include"rpc/abstract_publication_feed.hpp"
#include"mutex.hpp"

#include<vector>
#include <iterator>

namespace fetch {
namespace rpc {
// TODO: add signature testing in development mode  
class Feed {
public:
  /*
    @brief creates a feed that services can subscribe to.

    @param feed is the feed number defined in the protocol.
    @param publisher is an implementation class that subclasses AbstractPublicationFeed.
   */
  Feed(feed_handler_type const &feed, AbstractPublicationFeed* publisher) :
    feed_(feed), publisher_(publisher) { }

  /*
    @brief attaches a feed to a given service.

    @param service is a pointer to the service
    @tparam T is the type of the service
   */
  template< typename T >
  void AttachToService(T *service) {

    publisher_->create_publisher(feed_, [=](fetch::byte_array::ReferencedByteArray const&  msg) {
        serializer_type params;
        params << RPC_FEED << feed_;
        uint64_t p = params.Tell();
        params << subscription_handler_type(0); // placeholder
        
        params.Allocate( msg.size() ); 
        params.WriteBytes( msg.pointer(), msg.size() );
          
        subscribe_mutex_.lock();
        std::vector< std::size_t > remove;
        for(std::size_t i=0; i < subscribers_.size(); ++i) {
          auto &s = subscribers_[i];
          params.Seek(p);
          params << s.id;
          
          // Copy is important here as we reuse an existing buffer
          if(!service->Send(s.client, params.data().Copy())) {
            std::cout << "Removing lost node" << std::endl;
            remove.push_back(i);
          }
        }
        
        std::reverse(remove.begin(), remove.end());
        for(auto &i: remove)
          subscribers_.erase(std::next(subscribers_.begin(), i)); 
        subscribe_mutex_.unlock();
      });
    
  }


  void Subscribe(uint64_t const & client, subscription_handler_type const &id) {    
    subscribe_mutex_.lock();
    subscribers_.push_back({ client, id });
    subscribe_mutex_.unlock();
  }

  void Unsubscribe(uint64_t const & client,  subscription_handler_type const &id) {

    subscribe_mutex_.lock();    
    std::vector< std::size_t > ids;
    for(std::size_t i=0; i < subscribers_.size(); ++i)
      if((subscribers_[i].client == client) &&
         (subscribers_[i].id == id)) ids.push_back(i);

    std::reverse(ids.begin(), ids.end());
    for(auto &i: ids)
      subscribers_.erase(std::next(subscribers_.begin(), i));
    subscribe_mutex_.unlock();
    
  }
  
  
  /*
    @brief returns the feed type.
  */
  feed_handler_type const& feed() const { return feed_; }

  /*
    @brief returns the publisher.
  */  
  fetch::rpc::AbstractPublicationFeed* publisher() const { return publisher_; }
private:
  struct ClientSubscription {
    uint64_t client;
    subscription_handler_type id;
  };
  std::vector< ClientSubscription > subscribers_;
  fetch::mutex::Mutex subscribe_mutex_;
  
  feed_handler_type feed_;
  fetch::rpc::AbstractPublicationFeed* publisher_ = nullptr;
};

};
};

#endif
