#ifndef STORAGE_KEY_VALUE_INDEX_HPP
#define STORAGE_KEY_VALUE_INDEX_HPP
#include "storage/random_access_stack.hpp"
#include "storage/cached_random_access_stack.hpp"
#include "storage/key.hpp"
#include "crypto/sha256.hpp"

#include <cstring>
#include<deque>
#include<queue>
namespace fetch {
namespace storage {
 
template< std::size_t S = 256, std::size_t N = 64 >
struct KeyValuePair {
  KeyValuePair() {
    memset(this, 0, sizeof(decltype(*this)));
    parent = uint64_t(-1);
    //    left = uint64_t(-1);
    //    right = uint64_t(-1);        
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

  bool is_leaf() const { return split == S; }
  
  bool UpdateLeaf(uint64_t const &val, byte_array::ConstByteArray const &data) {
    crypto::SHA256 hasher;
    hasher.Reset();
    hasher.Update( data );
    hasher.Final( hash );
    value = val;

    return true;
  }
  
  bool UpdateNode(KeyValuePair const &left, KeyValuePair const &right) {
    crypto::SHA256 hasher;
    hasher.Reset();

    hasher.Update( left.hash , N );
    hasher.Update( right.hash , N );
    hasher.Final( hash );

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
  
  template <typename KV = KeyValuePair< >, typename D = CachedRandomAccessStack< KV, uint64_t > >
class KeyValueIndex : public D {
  struct UpdateTask {
    uint64_t priority;
    uint64_t element;
    bool operator==(UpdateTask const &other) const {
      return (priority == other.priority) && (other.element == element);
    }
    bool operator<(UpdateTask const &other) const {
      return priority < other.priority;
    }
  };
public:
  typedef uint64_t index_type;
  typedef D super_type;
  typedef KeyValuePair<> key_value_pair;  
  typedef typename key_value_pair::key_type key_type;

  ~KeyValueIndex() 
  {
    OnBeforeFlush();
  }
    
    
  void OnFileLoaded() override final {
    super_type::OnFileLoaded();
    
    root_ = super_type::header_extra();
  }

  void OnBeforeFlush() override final {
    if(!this->is_open()) return;
      
    super_type::SetExtraHeader(root_);
    
    std::unordered_map< uint64_t, uint64_t > depths;
    std::unordered_map< uint64_t, uint64_t > parents;    
    std::priority_queue<UpdateTask> q;

    for(auto &k : schedule_update_) {
      auto &kv = k.second;
      uint64_t last = k.first;
      uint64_t pid = kv.parent;
      key_value_pair parent;
      uint64_t depth = 1;
      
      while(pid != uint64_t(-1) ) {
        if(parents.find(last) != parents.end()) {
          depth += depths[last];
          break;
        }
        parents[last] = pid;

        super_type::Get(pid,parent);        
        last = pid;
        pid = parent.parent;
        ++depth;
      }

      // Adding root
      if(pid == uint64_t(-1)) {
        parents[last] = pid; 
      }
      
      last = k.first;
      while(parents.find(last) != parents.end()) {
        if(depths.find(last) != depths.end()) break;
        depths[last] = depth;
        q.push( {depth, last} );
        --depth;
        last = parents[last];
      }
    }
    
    UpdateTask task;
    while(!q.empty()) {
      task = q.top();
      q.pop();      
      
      key_value_pair element, left, right;
      super_type::Get(task.element, element);
      if(element.is_leaf()) {
        continue;
      }

      super_type::Get(element.left, left);
      super_type::Get(element.right, right);
      element.UpdateNode(left, right);
      super_type::Set(task.element, element);

    }

    schedule_update_.clear();
  }
  
  index_type Find(byte_array::ConstByteArray const &key_str) {
    key_type key(key_str);       
    bool split;
    int pos;
    key_value_pair kv;
    int left_right;
    index_type depth;
    return FindNearest(key, kv, split, pos, left_right, depth);
  }

  
  void Delete(byte_array::ConstByteArray const &key) {
  }

  void GetElement(uint64_t const &i, index_type &v) {
    key_value_pair p;
    super_type::Get(i, p);
    v = p.value;
  }
  
  index_type Get(byte_array::ConstByteArray const &key_str) {
    key_type key(key_str);   
    bool split;
    int pos;
    key_value_pair kv;
    int left_right;
    index_type depth;    
//    index_type index =
    FindNearest(key, kv, split, pos, left_right, depth);
    // if (index == -1) { throw ... }
    assert( ! split );
    return kv.value;
  }


  template< typename ... Args> // index_type const &value, 
  void Set(byte_array::ConstByteArray const &key_str, Args const &... args) {
    key_type key(key_str);   
    bool split;
    int pos;
    key_value_pair kv;
    int left_right;

    index_type depth;    
    index_type index =  FindNearest(key, kv, split, pos, left_right, depth);

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
      //      std::cout << ">>>>>> Splitting " << index << " " << root_ << " : " << " > " ;
      
      switch(left_right) {
      case -1:
        cid = rid = index;
        right = kv;

        pid = right.parent;
        
        left.key = key;
        left.split = uint16_t(key.size());
        
        left.parent = super_type::size() + 1;
        right.parent = super_type::size() + 1;
        
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
    }

    if((kv.parent != index_type(-1)) &&
       (update_parent) ) {
      if(super_type::DirectWrite()) {
        UpdateParents(kv.parent, index, kv);
      } else {
        schedule_update_[ index ] = kv;
      }
    }
  }


  byte_array::ByteArray Hash() {
    super_type::Flush();
    
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
      key_value_pair kv, parent;
      super_type::Get(nexts.front(), kv);

      if(kv.split < 256) {
        nexts.push_back(kv.left);
        nexts.push_back(kv.right);
      }

      // Computing depth
      std::size_t depth = 1, pid = kv.parent;
      while(pid != std::size_t(-1)) {
        ++depth;
        super_type::Get(pid, parent);
        pid = parent.parent;
      }
        
      std::cout << std::setw(2) << nexts.front() << ") ";
      PrintBits(kv.key.ToByteArray(), kv.split );
      nexts.pop_front();
      std::cout << " > " <<kv.split << " : ";
      std::cout <<   kv.parent << " " << depth << std::endl ;
      //      std::cout << byte_array::ToBase64( kv.Hash() ) << std::endl;
      std::cout <<"        - "<< kv.left << " " << kv.right << " " << kv.key.ToByteArray() << std::endl;
      ++k;
      //      if(k > 5 ) break;
    }
  }

  uint64_t const &root_element() const { return root_; }
private:
  uint64_t root_ = 0;
  std::unordered_map< uint64_t, key_value_pair > schedule_update_;
  
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
                         int &pos, int &left_right, uint64_t &depth) {
    depth = 0;
    if(this->empty()) return index_type(-1);
    

    std::size_t next = root_, index;
    do {
      ++depth;
      index = next;
      //      std::cout << "Comparing to " << index << std::endl;
      pos = int( key.size() );

      //      if(depth > 1000) exit(-1);
      super_type::Get(next, kv);

      
      left_right = key.Compare( kv.key, pos, kv.split >> 8, kv.split & 63 );
      /*
      std::cout << key.ToByteArray() << " vs " <<  kv.key.ToByteArray() << std::endl;
      std::cout << "Index: " << index << " " << left_right << " " << pos << " " << kv.split << std::endl;
      PrintBits( key.ToByteArray(), 256);
      
      std::cout << std::endl;
      PrintBits( kv.key.ToByteArray(), 256);
      std::cout << std::endl;
      */

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
