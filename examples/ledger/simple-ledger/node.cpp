#define FETCH_DISABLE_TODO_COUT
#include<iostream>
#include<functional>
#include"node_functionality.hpp"

#include"commandline/parameter_parser.hpp"
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

#include"crypto/hash.hpp"
#include"unittest.hpp"
using namespace fetch::commandline;

void TestTransaction() {

}

int main(int argc, char const **argv) {
  SCENARIO("basic input and output of the nodes chain manager") {
    NodeChainManager manager;
    typedef typename NodeChainManager::transaction_type tx_type;

    INFO("Creating transaction");
    tx_type tx;
    tx.set_body("hello world");

    SECTION("transaction should be valid and with right hash") {
      INFO("TODO: Should check signatures etc.");
      
      // Making output for comparison
      fetch::serializers::ByteArrayBuffer buf;
      buf << "hello world";
      EXPECT( tx.digest() == fetch::crypto::Hash< fetch::crypto::SHA256 >(buf.data())  );
    };

    SECTION_REF("Checking that transaction can only be added once") {
      EXPECT( manager.PushTransaction( tx ) == true );
      EXPECT( manager.PushTransaction( tx ) == false);
    };
    
  };
  return 0;
}
