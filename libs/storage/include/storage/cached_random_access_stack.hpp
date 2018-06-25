#ifndef STORAGE_CACHED_RANDOM_ACCESS_STACK_HPP
#define STORAGE_CACHED_RANDOM_ACCESS_STACK_HPP
#include <fstream>
#include <string>
#include<unordered_map>

#include "core/assert.hpp"

namespace fetch {

namespace storage {

template <typename T, typename D = uint64_t>
class CachedRandomAccessStack : private RandomAccessStack<T,D> {
 public:
  typedef RandomAccessStack<T,D> super_type;
  typedef D header_extra_type;
  typedef T type;

  ~CachedRandomAccessStack() {
    Flush();
  }

  virtual void OnFileLoaded() {

  }
  
  void Load(std::string const &filename) {
    super_type::Load(filename);
    total_access_ = 0;
    OnFileLoaded();
  }

  void New(std::string const &filename) {
    super_type::New(filename);
    Clear();
    total_access_ = 0;
    OnFileLoaded();
  }

  void Get(uint64_t const &i, type &object)  {
    ++total_access_;
    auto iter = data_.find(i);
    if(iter !=data_.end() ) {
      ++iter->second.reads;
      object = iter->second.data;
    } else {
      assert(false);
    }
  }

  void Set(uint64_t const &i, type const &object) {
    ++total_access_;
    auto iter = data_.find(i);
    if(iter !=data_.end() ) {
      ++iter->second.writes;
      iter->second.updated = true;
      iter->second.data = object;
    } else {
      assert(false);
      assert( i < objects_);
      CachedDataItem itm;
      itm.data = object;
      data_.insert(std::pair<uint64_t, CachedDataItem > (i, itm));
    }
  }

  /*
  void SetExtraHeader(header_extra_type const &he) {
    detailed_assert(filename_ != "");
    header_.extra = he;
  }
  */
  //  header_extra_type header_extra() const { return extra_; }

  uint64_t Push(type const &object) {
    ++total_access_;
    uint64_t ret = objects_;
    CachedDataItem itm;
    itm.data = object;
    itm.updated = true;
    data_.insert(std::pair<uint64_t, CachedDataItem > (ret, itm));
    ++objects_;

    return ret;
  }

  void Pop() {
    --objects_;
    data_.erase( objects_ );
  }

  type Top() const {
    return data_[objects_ - 1];
  }

  void Swap(uint64_t const &i, uint64_t const &j) {
    if (i == j) return;
    total_access_ += 2;
    data_.find(i)->second.updated = true;
    data_.find(j)->second.updated = true;
    std::swap( data_[i], data_[j]);
  }

  std::size_t size() const { return objects_; }
  std::size_t empty() const { return objects_ == 0; }

  void Clear() {
    super_type::Clear();
    objects_ = 0;
    data_.clear();
  }

  void Flush() {
    for(auto &item: data_) {
      if(item.second.updated) {
        if(item.first >= super_type::size()) {
          assert( item.first == super_type::size());
          super_type::LazyPush( item.second.data );
        } else {
          assert( item.first < super_type::size());
          super_type::Set( item.first, item.second.data );
        }
      }

    }
    super_type::StoreHeader();
    super_type::Flush();
    
    for(auto &item: data_ ) {

      item.second.reads = 0;
      item.second.writes = 0;
      item.second.updated = false;            
    }
    total_access_ = 0;
  }

protected:
  

 private:
  uint64_t total_access_;
  struct CachedDataItem {
    uint64_t reads = 0;
    uint64_t writes = 0;
    bool updated = false;
    type data;
  };
  
  std::map< uint64_t, CachedDataItem > data_;
  uint64_t objects_ = 0;
};
}
}

#endif
