#define FETCH_DISABLE_TODO_COUT
#include <functional>
#include <iostream>

#include "protocols/shard/manager.hpp"

#include "commandline/parameter_parser.hpp"
#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

#include "byte_array/encoders.hpp"
#include "crypto/hash.hpp"
#include "random/lfg.hpp"
#include "unittest.hpp"
using namespace fetch::protocols;
using namespace fetch::commandline;
using namespace fetch::byte_array;

typedef typename ShardManager::transaction_type tx_type;

std::vector<std::string> words = {
    "squeak",     "fork",       "governor", "peace",       "courageous",
    "support",    "tight",      "reject",   "extra-small", "slimy",
    "form",       "bushes",     "telling",  "outrageous",  "cure",
    "occur",      "plausible",  "scent",    "kick",        "melted",
    "perform",    "rhetorical", "good",     "selfish",     "dime",
    "tree",       "prevent",    "camera",   "paltry",      "allow",
    "follow",     "balance",    "wave",     "curved",      "woman",
    "rampant",    "eatable",    "faulty",   "sordid",      "tooth",
    "bitter",     "library",    "spiders",  "mysterious",  "stop",
    "talk",       "watch",      "muddle",   "windy",       "meal",
    "arm",        "hammer",     "purple",   "company",     "political",
    "territory",  "open",       "attract",  "admire",      "undress",
    "accidental", "happy",      "lock",     "delicious"};

fetch::random::LaggedFibonacciGenerator<> lfg;
tx_type                                   RandomTX(std::size_t const &n)
{

  std::string ret = words[lfg() & 63];
  for (std::size_t i = 1; i < n; ++i)
  {
    ret += " " + words[lfg() & 63];
  }
  tx_type t;
  t.set_body(ret);
  return t;
}

int main(int argc, char const **argv)
{
  SCENARIO("basic input and output of the nodes chain manager")
  {
    ShardManager manager;

    INFO("Creating transaction");
    tx_type tx;
    tx.set_body("hello world");

    SECTION("transaction should be valid and with right hash")
    {
      INFO("TODO: Should check signatures etc.");

      // Making output for comparison
      fetch::serializers::ByteArrayBuffer buf;
      buf << "hello world";
      EXPECT(tx.digest() ==
             fetch::crypto::Hash<fetch::crypto::SHA256>(buf.data()));
    };

    SECTION_REF("Checking that transaction can only be added once")
    {
      EXPECT(manager.PushTransaction(tx) == true);
      EXPECT(manager.PushTransaction(tx) == false);
    };

    auto  block  = manager.GetNextBlock();
    auto  block2 = block;
    auto &p      = block.proof();
    auto &p2     = block2.proof();
    p.SetTarget(17);

    SECTION_REF("Finding a prrof of for the last transaction")
    {

      // Proof of work search
      INFO("Finding proof");
      while (!p()) ++p;
      ++p2, p2();

      EXPECT(p.digest() < p.target());
      EXPECT(p.digest() == block.proof().digest());
      EXPECT(p.digest() != block2.proof().digest());
      EXPECT(p2.digest() == block2.proof().digest());

      manager.PushBlock(block2);
      manager.PushBlock(block);
    };

    SECTION_REF("building a chain")
    {
      std::vector<tx_type> all_txs;
      std::vector<tx_type> temp_txs;
      std::size_t          counterA = 0, counterB = 0;
      for (std::size_t i = 0; i < 100; ++i)
      {
        std::size_t N = lfg() & 7;

        for (std::size_t j = 0; j < N; ++j)
        {
          ++counterA;
          auto tx = RandomTX(4);
          manager.PushTransaction(tx);
          all_txs.push_back(tx);
          temp_txs.push_back(tx);
        }

        std::size_t M = lfg() & 7;
        for (std::size_t j = 0; j < M; ++j)
        {
          ++counterB;
          auto block = manager.GetNextBlock();
          if (temp_txs.size() != 0)
          {
            auto body             = block.body();
            body.transaction_hash = temp_txs[lfg() % temp_txs.size()].digest();
            block.SetBody(body);
          }

          auto &p = block.proof();

          p.SetTarget(lfg() % 5);
          while (!p()) ++p;

          manager.PushBlock(block);
        }

        manager.Commit();
      }

      std::cout << "TXs: " << counterA << ", blocks: " << counterB << std::endl;
    };
  };
  return 0;
}
