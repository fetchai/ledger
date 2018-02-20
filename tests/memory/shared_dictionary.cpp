#define FETCH_TESTING_ENABLED
#include<iostream>
#include<random/lcg.hpp>
#include<byte_array/referenced_byte_array.hpp>
#include<memory/shared_hashtable.hpp>
#include<random/lfg.hpp>

#include<unordered_map>
#include<vector>
using namespace fetch::memory;

typedef uint64_t data_type;
typedef SharedHashTable< data_type > dict_type;

std::vector< std::string > words = {"squeak", "fork", "governor", "peace", "courageous", "support", "tight", "reject", "extra-small", "slimy", "form", "bushes", "telling", "outrageous", "cure", "occur", "plausible", "scent", "kick", "melted", "perform", "rhetorical", "good", "selfish", "dime", "tree", "prevent", "camera", "paltry", "allow", "follow", "balance", "wave", "curved", "woman", "rampant", "eatable", "faulty", "sordid", "tooth", "bitter", "library", "spiders", "mysterious", "stop", "talk", "watch", "muddle", "windy", "meal", "arm", "hammer", "purple", "company", "political", "territory", "open", "attract", "admire", "undress", "accidental", "happy", "lock", "delicious"}; 

fetch::random::LaggedFibonacciGenerator<> lfg;
std::string RandomKey(std::size_t const &n) {

  std::string ret = words[ lfg() & 63 ];
  for(std::size_t i=1; i < n;++i) {
    ret += " " + words[lfg() & 63];
  }
  
  return ret;
}

struct Hash {
  size_t operator() (fetch::byte_array::BasicByteArray const  &key) const {
    uint32_t hash = 2166136261;
    for (std::size_t i = 0; i < key.size(); ++i)
    {
      hash = (hash * 16777619) ^ key[i];
    }

    return hash ;    
  }
};

int main() {
  std::vector< fetch::byte_array::ByteArray > keys;
  for(std::size_t i = 0; i < 1000; ++i)
  {
    keys.push_back(RandomKey( 4  ) );    
  }
  


  auto start = std::chrono::high_resolution_clock::now();  
  dict_type dct(16);
  std::cout << "Was here? " << std::endl;
  
  for(auto const &k : keys)
  {
    dct[k] = lfg();
  }
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> diff = end-start;
  std::cout << "Time: " << diff.count() << " s\n";  

  

  start = std::chrono::high_resolution_clock::now();  

  std::unordered_map< fetch::byte_array::BasicByteArray, data_type, Hash > ref;
  

  for(auto const &k : keys)
  {
    ref[k] = lfg();
  }
  end = std::chrono::high_resolution_clock::now();
  diff = end-start;
  std::cout << "Time: " << diff.count() << " s\n";  
  
  
  return 0;
}
