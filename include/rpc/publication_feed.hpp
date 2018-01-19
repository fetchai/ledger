/**
   @brief implementation of publication functionality.
   
   A class can inherith this functionality to create and publish to
   feeds that can then later be added to protocols. Consider a message
   passing protocol. In this protocol we would like the underlying
   functionality to provide a feed with new messages. This can be done
   as follows:

     enum MessageFeed {  
       NEW_MESSAGE = 1 
     };   
     
     class MessageManager : public HasPublicationFeed {
     public:
       // ...
       void PushMessage(std::string const &msg) {
          messages_.push_back(msg);
          this->Publish( MessageFeed::NEW_MESSAGE, msg );
       }
       // ...
     };
   
   In the protocol definition we can now send the ... TODO
**/  
#ifndef RPC_PUBLICATION_FEED_HPP
#define RPC_PUBLICATION_FEED_HPP
#include"rpc/abstract_callable.hpp"
#include"rpc/abstract_publication_feed.hpp"
#include"assert.hpp"

#include<functional>

namespace fetch {
namespace rpc {


class HasPublicationFeed : public AbstractPublicationFeed {  
public:

  /**
     @see AbstractPublicationFeed for documentation details.
   **/
  void create_publisher(feed_handler_type feed, function_type function) override {
    if(publisher_[feed]) {
      TODO_FAIL("FeedEvents does not have support for multiple publishers. Please use MultiFeedEvents");
    }
    publisher_[feed] = function;
  }

  /**
     @brief publishes data to a feed.

     A class that implements a given functionality would normally
     subclass this class and use the this function to publish data. Data
     feeds are separated such that one can use them in multiple
     protocols. For instance, one could make a UDP feed for new blocks
     while having another protocol that would publish messages over TCP.
     
     @param feed is the to which this is published.
     @param ...args is the argument list that will be published.
  **/  
  template< typename ...Args >
  void Publish(feed_handler_type feed, Args &&...args ) {
    serializer_type params;

    PackArgs(params, args...);

    if(publisher_[feed])
      publisher_[feed](params.data());    
    else {
      TODO_FAIL("could not find publisher - make a warning logger");
    }
  }
private:
  function_type publisher_[256] ;
};

};
};
#endif
