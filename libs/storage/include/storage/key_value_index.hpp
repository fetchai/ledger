#ifndef STORAGE_KEY_VALUE_INDEX_HPP
#define STORAGE_KEY_VALUE_INDEX_HPP
#include "storage/random_access_stack.hpp"
#include "storage/cached_random_access_stack.hpp"
#include "storage/versioned_random_access_stack.hpp"
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
  
template <typename KV = KeyValuePair< >, typename D = VersionedRandomAccessStack< KV, uint64_t > >
class KeyValueIndex {
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
  typedef D stack_type;
  typedef KeyValuePair<> key_value_pair;  
  typedef typename key_value_pair::key_type key_type;
    KeyValueIndex() 
    {
      
      stack_.OnFileLoaded([this]() {
          root_ = stack_.header_extra();
        });
      stack_.OnBeforeFlush([this]() {
          this->BeforeFlushHandler();
        });      
    }
    
    
  ~KeyValueIndex() 
  {
    stack_.ClearEventHandlers();    
    BeforeFlushHandler();
  }

    template<typename... Args>
    void New(Args &&...args) 
    {
      stack_.New(std::forward<Args>(args)...);      
    }
    
    template<typename... Args>
    void Load(Args &&...args) 
    {
      stack_.Load(std::forward<Args>(args)...);      
    }
    
    
  void BeforeFlushHandler() {
    if(!this->is_open()) return;
      
    stack_.SetExtraHeader(root_);
    
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

        stack_.Get(pid,parent);        
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
      stack_.Get(task.element, element);
      if(element.is_leaf()) {
        continue;
      }

      stack_.Get(element.left, left);
      stack_.Get(element.right, right);
      element.UpdateNode(left, right);
      stack_.Set(task.element, element);

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
    TODO_FAIL("Not implemented");
  }

  void GetElement(uint64_t const &i, index_type &v) {
    key_value_pair p;
    stack_.Get(i, p);
    v = p.value;
  }

  bool GetIfExists(byte_array::ConstByteArray const &key_str, index_type &value) 
  {
    key_type key(key_str);   
    bool split = true;
    int pos = 0;
    key_value_pair kv;
    int left_right = 0;
    index_type depth = 0;    
    FindNearest(key, kv, split, pos, left_right, depth);

    
    if(!split) {
      value = kv.value;
    }
    
    return ! split ;
  }   
    
  index_type Get(byte_array::ConstByteArray const &key_str) {
    key_type key(key_str);   
    bool split;
    int pos;
    key_value_pair kv;
    int left_right;
    index_type depth;    
    FindNearest(key, kv, split, pos, left_right, depth);
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
      kv.key = key;
      kv.parent = uint64_t(-1);
      kv.split = uint16_t(key.size());
      update_parent = kv.UpdateLeaf( args... );

      index = stack_.Push( kv );
    } else if(split) {
      
      key_value_pair left, right, parent;
      index_type rid, lid, pid, cid;
      bool update_root = (index == root_);
      
      switch(left_right) {
      case -1:
        cid = rid = index;
        right = kv;

        pid = right.parent;
        
        left.key = key;
        left.split = uint16_t(key.size());
        
        left.parent = stack_.size() + 1;
        right.parent = stack_.size() + 1;
        
        update_parent = left.UpdateLeaf( args... );
        
        lid = stack_.Push(left);
        stack_.Set(rid, right);
        break;
      case 1:
        cid = lid = index;
        left = kv;

        pid = left.parent;        
        
        right.key = key;
        right.split = uint16_t(key.size());
                
        right.parent = stack_.size() + 1;
        left.parent = stack_.size() + 1;         

        update_parent = right.UpdateLeaf( args... );
        
        rid = stack_.Push(right);
        stack_.Set(lid, left);
        break;        
      }

      
      kv.split = uint16_t(pos);
      kv.left = lid;
      kv.right = rid;
      kv.parent = pid;
      index =  stack_.Push( kv );

      
      if(update_root) {
        root_ = index;
      } else {
        stack_.Get(pid, parent );
        if(parent.left == cid) {
          parent.left = index;
        } else {
          parent.right = index;
        }
        stack_.Set(pid, parent );
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
      update_parent = kv.UpdateLeaf( args... );
      stack_.Set(uint64_t(index), kv);
    }

    if((kv.parent != index_type(-1)) &&
       (update_parent) ) {
      if(stack_.DirectWrite()) {
        UpdateParents(kv.parent, index, kv);
      } else {
        schedule_update_[ index ] = kv;
      }
    }
  }


  byte_array::ByteArray Hash() {
    stack_.Flush();
    key_value_pair kv;
    
    if(stack_.size() > 0) {
      stack_.Get(root_, kv);
    }
    
    return kv.Hash();
  }

    std::size_t size() const 
    {
      return stack_.size();
    }

    void Flush()
    {
      stack_.Flush();
    }

    bool is_open() const 
    {
      return stack_.is_open();
    }

    bool empty() const 
    {
      return stack_.empty();
    }

    void Close() 
    {
      stack_.Close();
    }
    
  typedef uint64_t bookmark_type;
  bookmark_type Commit() {
    return stack_.Commit();
  }

  bookmark_type Commit(bookmark_type const& b) {
    return stack_.Commit(b);
  }  
  
  void Revert(bookmark_type const &b) {
    stack_.Revert(b);
  }

  uint64_t const &root_element() const { return root_; }
private:
  stack_type stack_;
    
  uint64_t root_ = 0;
  std::unordered_map< uint64_t, key_value_pair > schedule_update_;
  
  void UpdateParents(index_type pid, index_type cid, key_value_pair child) {
    key_value_pair parent, left, right;

    while(pid != index_type(-1)) {
      stack_.Get(pid, parent);
      if(cid == parent.left) {
        left = child;
        stack_.Get(parent.right, right);
      } else {
        right = child;
        stack_.Get(parent.left, left);  
      }

      parent.UpdateNode(left, right);
      stack_.Set(pid, parent);
      
      child = parent;
      cid = pid;
      pid = child.parent;
    }
  }

  
  index_type FindNearest(key_type const &key, key_value_pair &kv, bool &split,
                         int &pos, int &left_right, uint64_t &depth) {
    depth = 0;
    if(this->empty()) return index_type(-1);
    

    std::size_t next = root_, index;
    do {
      ++depth;
      index = next;
      pos = int( key.size() );

      stack_.Get(next, kv);

      left_right = key.Compare( kv.key, pos, kv.split >> 8, kv.split & 63 );

      switch(left_right) {
      case -1:
        next = kv.left;
        break;
      case 1:
        next = kv.right;
        break;
      }
    } while( (left_right != 0) && (pos >= int(kv.split) ));

    split = (left_right != 0) && (pos <  int(kv.split));
    return index;
  }
  
};

}
}

#endif
