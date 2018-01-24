#ifndef CHAIN_CONSENSUS_PROOF_OF_WORK_HPP
#define CHAIN_CONSENSUS_PROOF_OF_WORK_HPP
#include"crypto/sha256.hpp"
namespace fetch {
namespace consensus {
  
class ProofOfWork {
public:
  bool operator<(ProofOfWork const &other) {
    
  }

private:
  digest_type digest_;
  digest_type nonce_;  
};
  
};
};

#endif
