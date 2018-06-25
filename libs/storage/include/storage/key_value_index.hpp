#ifndef STORAGE_KEY_VALUE_INDEX_HPP
#define STORAGE_KEY_VALUE_INDEX_HPP
#include "storage/random_access_stack.hpp"
#include "storage/cached_random_access_stack.hpp"
#include "storage/key.hpp"
#include "crypto/sha256.hpp"

#include <cstring>
#include<deque>
namespace fetch {
namespace storage {
 
template< std::size_t S = 256, std::size_t N = 64 >
struct KeyValuePair {
  KeyValuePair() {
    memset(this, 0, sizeof(decltype(*this)));
    parent = uint64_t(-1);
    left = uint64_t(-1);
    right = uint64_t(-1);        
  }
  
  typedef Key< S > key_type;

  key_type key;
  uint8_t hash[N];
  
  uint16_t split;

  uint64_t parent;
  union {
    uint64_t value;
    uint64_t left;
  };
  uint64_t right;

  bool UpdateLeaf(uint64_t const &val, byte_array::ConstByteArray const &data) {
    crypto::SHA256 hasher;
    hasher.Reset();
    hasher.Update( data );
    hasher.Final( hash );
    value = val;

    //    std::cout << " >>>>>>>>>>>>>>>> -------------------------------------------------------- +++++++ " << byte_array::ToBase64( Hash() ) << std::endl;    
    return true;
  }
  
  bool UpdateNode(KeyValuePair const &left, KeyValuePair const &right) {
    crypto::SHA256 hasher;
    hasher.Reset();
    //    std::cout << " -- L: " << byte_array::ToBase64( left.Hash() ) << std::endl;
    //    std::cout << " -- R: " << byte_array::ToBase64( right.Hash() ) << std::endl;

    hasher.Update( left.hash , N );
    hasher.Update( right.hash , N );
    hasher.Final( hash );
    //    std::cout << " >>>>> " << byte_array::ToBase64( Hash() ) << std::endl;
    return true;
  }

  byte_array::ByteArray Hash() const {
    byte_array::ByteArray ret;
    ret.Resize( N );
    
    for(std::size_t i=0; i < N; ++i) {
      ret[i] = hash[i];
    }
    
    return ret;    
  }
    
};
  
template <typename KV = KeyValuePair< >, typename D = CachedRandomAccessStack< KV > >
class KeyValueIndex : public D {

public:
  typedef uint64_t index_type;
  typedef D super_type;
  typedef KeyValuePair<> key_value_pair;  
  typedef typename key_value_pair::key_type key_type;

  void OnFileLoaded() override {
    // TODO: Load index from header
    root_ = 0;
  }
  
  index_type Find(byte_array::ConstByteArray const &key_str) {
    key_type key(key_str);       
    bool split;
    int pos;
    key_value_pair kv;
    int left_right;
    return FindNearest(key, kv, split, pos, left_right);
    
  }
                                                     
  
  void Delete(byte_array::ConstByteArray const &key) {
  }

  index_type Get(byte_array::ConstByteArray const &key_str) {
    key_type key(key_str);   
    bool split;
    int pos;
    key_value_pair kv;
    int left_right;
    //    index_type index =
    FindNearest(key, kv, split, pos, left_right);

    //    std::cout << "Found at " << index << std::endl;
    /*
    if( (index == -1) || (split) ) {
      std::cerr <<"FAILED: " << split << " " << index <<  std::endl;
      exit(-1);
    }
    */
    return kv.value;
  }


  template< typename ... Args> // index_type const &value, 
  void Set(byte_array::ConstByteArray const &key_str, Args const &... args) {
    key_type key(key_str);   
    bool split;
    int pos;
    key_value_pair kv;
    int left_right;
    index_type index =  FindNearest(key, kv, split, pos, left_right);
    index_type first_parent = index_type(-1);
    bool update_parent = false;

    if(index == index_type(-1)) {
      //      std::cout << ">>>>>> Adding root " << std::endl;
      kv.key = key;
      kv.parent = uint64_t(-1);
      kv.split = uint16_t(key.size());
      update_parent = kv.UpdateLeaf( args... );

      index = super_type::Push( kv );
    } else if(split) {
      
      key_value_pair left, right, parent;
      index_type rid, lid, pid, cid;
      bool update_root = (index == root_);
      //      std::cout << ">>>>>> Splitting " << index << " " << root_ << std::endl;
      switch(left_right) {
      case -1:
        cid = rid = index;
        right = kv;   
        pid = right.parent;
        
        left.key = key;
        left.split = uint16_t(key.size());

        right.parent = super_type::size() + 1;
        left.parent = super_type::size() + 1; 
        update_parent = left.UpdateLeaf( args... );
        
        lid = super_type::Push(left);
        super_type::Set(rid, right);
        break;
      case 1:
        cid = lid = index;
        left = kv;
        pid = left.parent;        
        
        right.key = key;
        right.split = uint16_t(key.size());
        
        right.parent = super_type::size() + 1;
        left.parent = super_type::size() + 1;         

        update_parent = right.UpdateLeaf( args... );
        
        rid = super_type::Push(right);
        super_type::Set(lid, left);
        break;        
      }


      kv.split = uint16_t(pos);
      kv.left = lid;
      kv.right = rid;
      kv.parent = pid;
      index =  super_type::Push( kv );
      first_parent = index;
      
      if(update_root) {
        //        std::cout<< "Setting new root " << index << " " << pid << std::endl;           
        root_ = index;
      } else {
        //        std::cout << "Writing parent: " << pid << std::endl;
        super_type::Get(pid, parent );
        if(parent.left == cid) {
          parent.left = index;
        } else {
          parent.right = index;
        }
        super_type::Set(pid, parent );
      }

      switch(left_right) {
      case -1:
        index = kv.left;
        kv = left;
        break;
      case 1:
        index = kv.right;        
        kv = right;
        break;
      }
    } else {
      //      std::cout << ">>>>>> Updating " << std::endl;
      update_parent = kv.UpdateLeaf( args... );
      super_type::Set(uint64_t(index), kv);
      first_parent = kv.parent;
    }

    if((first_parent != index_type(-1)) &&
       (update_parent) ) {
      UpdateParents(first_parent, index, kv);
    }
  }


  byte_array::ByteArray Hash() {
    key_value_pair kv;
    super_type::Get(root_, kv);
    return kv.Hash();
  }

  void PrintBits(byte_array::ByteArray const &b, uint64_t split) {
    split = b.size() * 8 - split ;
    for(std::size_t i=b.size() / 2 ; i < b.size(); ++i) {
      std::size_t k = i * 8 ;
      uint8_t c = b[b.size() - 1 - i];
      for(std::size_t j=0; j < 8; ++j) {
        if(k >= split)
          std::cout << ((c >> (7 - j)) & 1) ;
        else
          std::cout << "-";
        ++k;
      }
      std::cout << " ";
    }
    //    std::cout << std::endl;
  }
  
  void PrintTree() {
    std::deque< uint64_t > nexts;
    nexts.push_back(root_);
    std::size_t k = 0;
    while(!nexts.empty()) {
      key_value_pair kv;
      super_type::Get(nexts.front(), kv);

      if(kv.split < 256)
        nexts.push_back(kv.left);
      if(kv.split < 256)   
        nexts.push_back(kv.right);
      
      std::cout << std::setw(2) << nexts.front() << ") ";
      PrintBits(kv.key.ToByteArray(), kv.split );
      nexts.pop_front();
      std::cout << " > " <<kv.split << " : ";
      std::cout << kv.left << " " << kv.right << " " <<   kv.parent << " " <<  std::endl ;
      //      std::cout << byte_array::ToBase64( kv.Hash() ) << std::endl;
      std::cout << kv.key.ToByteArray() << std::endl;
      ++k;
      //      if(k > 5 ) break;
    }
  }
  
private:
  uint64_t root_ = 0;

  
  void UpdateParents(index_type pid, index_type cid, key_value_pair child) {
    key_value_pair parent, left, right;
    //    std::cout << "UP: " << pid << std::endl;
    while(pid != index_type(-1)) {
      super_type::Get(pid, parent);
      if(cid == parent.left) {
        left = child;
        super_type::Get(parent.right, right);
      } else {
        right = child;
        super_type::Get(parent.left, left);  
      }
      //      std::cout << "Updating " << pid << " " << parent.key.ToByteArray() << std::endl;
      parent.UpdateNode(left, right);
      super_type::Set(pid, parent);
      
      child = parent;
      cid = pid;
      pid = child.parent;
      //      std::cout << "Next: " << pid << std::endl;
    }
    //    std::cout << " ---- " << std::endl << std::endl;
  }

  
  index_type FindNearest(key_type const &key, key_value_pair &kv, bool &split,
                         int &pos, int &left_right) {
    if(this->empty()) return index_type(-1);
    
    std::size_t n = 0;
    std::size_t next = root_, index;
    do {
      index = next;
      //      std::cout << "Comparing to " << index << std::endl;
      pos = int( key.size() );
      super_type::Get(next, kv);
      /*
      std::cout << "KV : " << std::endl;
      std::cout << "  - split: " << kv.split << std::endl;
      std::cout << "  - left:  " << kv.left << std::endl;
      std::cout << "  - right: " << kv.right << std::endl;
      std::cout << "  - parent: " << kv.parent << std::endl;            
      */
      left_right = key.Compare( kv.key, pos, kv.split >> 8, kv.split & 63 );
      /*
      std::cout << key.ToByteArray() << " vs " <<  kv.key.ToByteArray() << std::endl;
      std::cout << "Index: " << index << " " << left_right << " " << pos << " " << kv.split << std::endl;
      PrintBits( key.ToByteArray(), 256);
      
      std::cout << std::endl;
      PrintBits( kv.key.ToByteArray(), 256);
      std::cout << std::endl;
      */
      ++n;

      switch(left_right) {
      case -1:
        next = kv.left;
        break;
      case 1:
        next = kv.right;
        break;
      }
      //            std::cout << std::endl;
    } while( (left_right != 0) && (pos >= int(kv.split) ));

    split = (left_right != 0) && (pos <  int(kv.split));
    //    std::cout << " ---- DONE ------ : " <<  std::endl  << std::endl;
    return index;
  }
  
};

}
}

#endif
