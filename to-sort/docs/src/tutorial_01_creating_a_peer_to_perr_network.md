#<cldoc:Developers Guide::Tutorial::Creating a peer to peer network>

In this tutorial we go through the concepts of RPC, data feed and serialization.


In this tutorial we explore a protocol framework inside Fetch that
allows us to create protocols between AEA and the nodes as well as in
between the nodes themselves. We will as an example consider a system in
which data flows freely from AEAs to the nodes as well as in between the
nodes. Other AEAs can now buy the data from the nodes and the nodes
themselves can apply <Developers
Guide::Technology::Proof-of-useful-work> to derive new insightful data
gthat, in turn, can be resold. 

# Building the RC to node protocol - Part 1 
In the first part of the tutorial, we will build a protocol for the
remote controls (RCs) to allows us to interact with the nodes. The
functions we want to be able to invoke are: 

^ Operation           ^ Effect                                                           ^
| Connect to          | Asks the node to connect to another node                         |
| Get info            | Get information about the node                                   |
| Disconnect          | Disconnects gracefully from another node                         |
| List peers          | Returns a list of peer connections                               |

As we work our way through these three simple commands, we will learn how to define a protocol, deploy it over a network and serialize data structures.

## The backend implementation 
As we are still lacking the peer-to-peer network, we will in this first
tutorial make a simplified model where we only have a client and a
server. The RC is the client and the node is the server. The server has
a list of data types, storage for the data and a method to receive the
data. We will keep all of this in memory for now. For the sake of
simplicity, we will assume that all data are of type `double`. It is not
hard to redo this tutorial uses `Variants` which is part of the Fetch
ledgers core implementation.  

Let's assume that our backend implementation is as follows:

```
  #include<map>
  #include<vector>
  #include<string>
  #include<iostream>
  
  class NodeFunctionality {
  public:
    bool RegisterType(std::string name, std::string schema) {
      if(schemas_.find(name) != schemas_.end() ) return false;
      schemas_[name] = schema;
      std::cout << "Registered schema " << name << std::endl;
      return true;
    }
    
    void PushData(std::string name, std::vector< double > data) {
      if(schemas_.find(name) == schemas_.end()) return;
      
      if(data_.find(name) == data_.end() ) {
        std::cout << "Created new data set " << name << std::endl;
        data_[name] = std::vector< double >();
      }
      
      for(auto &a: data)
        data_[name].push_back(a);
      
      std::cout << "Added " << data.size() << " data points to " <<  name << std::endl;    
    }
    
    std::vector< double > PurchaseData(std::string name) {
      if(schemas_.find(name) == schemas_.end()) return std::vector< double >();
      if(data_.find(name) == data_.end()) return std::vector< double >();
      std::cout << "Sold " << data_.size() << " data points of type " <<  name << std::endl;        
      return data_[name];
    }
  
  private:
    std::map< std::string, std::string > schemas_;
    std::map< std::string, std::vector< double > > data_;  
  };
```

At this point, it would be appropriate to write a unit test that would verify the functionality of our implementation. 

## Creating the protocol 
After having verified that this is actually the behavior we want, we move on to expose this functionality through a protocol definition. The definition consists of two files, one that defines the operations we can do and a second one that links these operations to the functionality. The one that defines the operations is a simple file that only contains two `enums`:

```
  enum Commands {
    REGISTER  = 1,
    PUSH_DATA = 2,
    PURCHASE  = 3
  };
  
  enum Protocol {
    RC_PROTOCOL = 1
  };
```

The reason why this file is separated from the rest of the protocol definition is that it is needed on both the client and server side. Next, we define the protocol as a separate object as follows:

```
  #include"node_functionality.hpp"
  #include"commands.hpp"
  
  #include"rpc/service_server.hpp"
  
  class RCProtocol : public NodeFunctionality, public fetch::rpc::Protocol { 
  public:
    RCProtocol() : NodeFunctionality(),  Protocol() {
  
      using namespace fetch::rpc;
      auto register_function =  new CallableClassMember<NodeFunctionality, bool(std::string, std::string)>(this, &NodeFunctionality::RegisterType);
      
      // ... more to come
      
      this->Expose(REGISTER, register_function);
      
      // ... more to come
    }
  };
```

In the above, we ask the Fetch ledger library to expose the member function from `NodeFunctionality`. Because of how C++ is designed, we cannot automatically extract the function signature, so this have state explicitly as a template parameter `bool(std::string, std::string)`.

## Attaching the protocol to a service 
We attach the protocol to a service as the next step. A service is providing the transport layer for the communication. In our case, this is done using TCP, but one could as well make a UDP or other implementation. The service layer is not limited to using network for transport.

Since we only have one protocol, it is very simple to create a service with that protocol:

```
  class FetchService : public fetch::rpc::ServiceServer {
  public:
    RCService(uint16_t port) : ServiceServer(port) {
      this->Add(Protocol::RC_PROTOCOL, new RPCProto() );
    }
  };
```

And we are now ready to deploy the service, although it only exposes one of the three functionalities we listed above. 

## Server and client implementations 

To build the service we make a main file that looks as follows:

```
  #include<iostream>
  #include"node.hpp"
  
  int main() {
    FetchService serv(8080);
    serv.Start();
  
    std::string dummy;
    std::cout << "Press ENTER to quit" << std::endl;                                       
    std::cin >> dummy;
    
    serv.Stop();
  
    return 0;
  }
```

To test our server we make a simple client as follows:

```
  #include"commands.hpp"
  #include"rpc/service_client.hpp"
  
  #include<iostream>
  using namespace fetch::rpc;  
  
  int main(int argc, char const **argv) {
    if(argc != 3) {
      std::cerr << "usage: ./" << argv[0] << " [name] [schema]" << std::endl;
      exit(-1);
    }
    
    ServiceClient client("localhost", 8080);
    client.Start();
    std::this_thread::sleep_for( std::chrono::milliseconds(100) );
  
    auto p = client.Call( FetchProtocols::RC, RCCommands::REGISTER, argv[1], argv[2]  );
    if( bool(p) ) {
      std::cout << "Successfully added schema" << std::endl;
    } else {
      std::cout << "Schema already exists" << std::endl;    
    }
  
    client.Stop(); 
    
    return 0;
  }
```

After compiling, we can test the code by setting up the server and running the client in each their terminal:

```
  $ ./examples/tutorial_aea_to_node_client hello '{ "my": "schema" }' 
  Successfully added schema
  $ ./examples/tutorial_aea_to_node_client hello '{ "my": "schema" }' 
  Schema already exists
```

The server terminal:

```
  $ ./examples/tutorial_aea_to_node_server 
  Press ENTER to quit
  Registered schema hello
```

## Making implementations using constant reference 

You will note that none of the functions make use of `const &`. This is on purpose as this is not support by the library currently. There is planned support for `const &` in the future.



