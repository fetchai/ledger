#ifndef STORAGE_VERSIONED_RANDOM_ACCESS_STACK_HPP
#define STORAGE_VERSIONED_RANDOM_ACCESS_STACK_HPP
#include "storage/random_access_stack.hpp"
#include "storage/cached_random_access_stack.hpp"
#include "storage/variant_stack.hpp"

#include <cstring>
namespace fetch {
namespace storage {

template< typename B >
struct BookmarkHeader 
{
  B header;
  uint64_t bookmark;  
};  


template <typename T,  typename B = uint64_t, typename S = RandomAccessStack<T, BookmarkHeader< B> > >
class VersionedRandomAccessStack {
 private:
  typedef S stack_type;
  typedef B header_extra_type;
  typedef BookmarkHeader< B > header_type;
  
  struct HistoryBookmark {
    HistoryBookmark() { memset(this, 0, sizeof(decltype(*this))); }
    HistoryBookmark(B const &val) {
      memset(this, 0, sizeof(decltype(*this)));
      bookmark = val;
    }

    enum { value = 0 };
    B bookmark = 0;
  };

  struct HistorySwap {
    HistorySwap() { memset(this, 0, sizeof(decltype(*this))); }

    HistorySwap(uint64_t const &i_, uint64_t const &j_) {
      memset(this, 0, sizeof(decltype(*this)));
      i = i_;
      j = j_;
    }

    enum { value = 1 };
    uint64_t i = 0;
    uint64_t j = 0;
  };

  struct HistoryPop {
    HistoryPop() { memset(this, 0, sizeof(decltype(*this))); }

    HistoryPop(T const &d) {
      memset(this, 0, sizeof(decltype(*this)));
      data = d;
    }

    enum { value = 2 };
    T data;
  };

  struct HistoryPush {
    HistoryPush() { memset(this, 0, sizeof(decltype(*this))); }

    enum { value = 3 };
  };

  struct HistorySet {
    HistorySet() { memset(this, 0, sizeof(decltype(*this))); }

    HistorySet(uint64_t const &i_, T const &d) {
      memset(this, 0, sizeof(decltype(*this)));
      i = i_;
      data = d;
    }

    enum { value = 4 };
    uint64_t i = 0;
    T data;
  };

  struct HistoryHeader {
    HistoryHeader() { memset(this, 0, sizeof(decltype(*this))); }

    HistoryHeader(B const &d) {
      memset(this, 0, sizeof(decltype(*this)));
      data = d;
    }

    enum { value = 5 };
    B data;
  };
  
  
 public:
  typedef typename RandomAccessStack<T>::type type;
  typedef B bookmark_type;

  typedef std::function< void() > event_handler_type;

  event_handler_type on_file_loaded_;
  event_handler_type on_before_flush_;

  VersionedRandomAccessStack() 
  {
    stack_.OnFileLoaded([this]() {
        SignalFileLoaded() ;
      });
    stack_.OnBeforeFlush([this]() {
        SignalBeforeFlush() ;
      });    
  }
  
  ~VersionedRandomAccessStack() 
  {
    stack_.ClearEventHandlers();
  }
  
  
  void ClearEventHandlers() 
  {
    on_file_loaded_ = nullptr;
    on_before_flush_ = nullptr;
  }

  void OnFileLoaded(event_handler_type const &f) {
    on_file_loaded_ = f;    
  }
  
  void OnBeforeFlush(event_handler_type const &f) {
    on_before_flush_ = f;
  }

  void SignalFileLoaded() {
    if(on_file_loaded_) on_file_loaded_();
  }
  
  void SignalBeforeFlush() 
  {
    if(on_before_flush_) on_before_flush_();    
  }

  static constexpr bool DirectWrite() { return stack_type::DirectWrite(); }
  
  void Load(std::string const &filename, std::string const &history, bool const &create_if_not_exist = true) {
    stack_.Load(filename, create_if_not_exist);
    history_.Load(history,  create_if_not_exist);
    bookmark_ = stack_.header_extra().bookmark;
  }

  void New(std::string const &filename, std::string const &history) {
    stack_.New(filename);
    history_.New(history);
    bookmark_ = stack_.header_extra().bookmark;
  }

  void Clear() {
    stack_.Clear();
    history_.Clear();
    ResetBookmark();
    bookmark_ = stack_.header_extra().bookmark;
  }

  type Get(std::size_t const &i) const {
    type object;
    stack_.Get(i, object);
    return object;
  }

  void Get(std::size_t const &i, type &object) const {
    stack_.Get(i, object);
  }

  void Set(std::size_t const &i, type const &object) {
    type old_data;
    stack_.Get(i, old_data);
    history_.Push(HistorySet(i, old_data), HistorySet::value);
    stack_.Set(i, object);
  }

  uint64_t Push(type const &object) {
    history_.Push(HistoryPush(), HistoryPush::value);
    return stack_.Push(object);
  }

  void Pop() {
    type old_data = stack_.Top();
    history_.Push(HistoryPop(old_data), HistoryPop::value);
    stack_.Pop();

  }
  
  type Top() const { return stack_.Top(); }

  void Swap(std::size_t const &i, std::size_t const &j) {
    history_.Push(HistorySwap(i, j), HistorySwap::value);
    stack_.Swap(i, j);
  }

  void Revert(bookmark_type const &b) {
    
    while ((!empty()) && (b != bookmark_)) {
      std::cout << " -- next: ";
      
      if(!history_.empty()) {
        
        uint64_t t = history_.Type();
        

        switch (t) {
        case HistoryBookmark::value:
          std::cout << "bookmark";
          RevertBookmark();
          if( bookmark_ != b) {
            std::cout << " - found!";
            history_.Pop();
          }
          
          break;
        case HistorySwap::value:
          std::cout << "swap";
          RevertSwap();
          history_.Pop();
          break;
        case HistoryPop::value:
          std::cout << "pop";          
          RevertPop();
          history_.Pop();
          break;
        case HistoryPush::value:
          std::cout << "push";
          
          RevertPush();
          history_.Pop();
          break;
        case HistorySet::value:
          std::cout << "set";
          
          RevertSet();
          history_.Pop();
          break;
        case HistoryHeader::value:
          std::cout << "header";
          
          RevertHeader();
          history_.Pop();
          break;          
        default:
          TODO_FAIL("Problem: undefined type");
        }
        
      } else {
        std::cout << "history is empty";
      }
      std::cout << std::endl;
    }

    header_type h = stack_.header_extra();
    h.bookmark = bookmark_;    
    stack_.SetExtraHeader(h);

    NextBookmark();    
  }

  void SetExtraHeader(header_extra_type const &b) 
  {
    header_type h = stack_.header_extra();
    history_.Push(HistoryHeader(h.header), HistoryHeader::value);
    std::cout << "New header: " << h.header << " > " << b << std::endl;
    
    h.header = b;    
    stack_.SetExtraHeader(h);
  }
  
  header_extra_type const &header_extra() const
  {
    return stack_.header_extra().header;
  }
  
  bookmark_type Commit() {
    bookmark_type b = bookmark_;

    return Commit(b);
  }

  bookmark_type Commit(bookmark_type const &b) {
    
    history_.Push(HistoryBookmark(b), HistoryBookmark::value);

    header_type h = stack_.header_extra();
    h.bookmark = bookmark_ = b; 
    stack_.SetExtraHeader(h);
    NextBookmark();    
    return b;
  }

  
  void Flush() 
  {
    stack_.Flush();
  }
  
  void ResetBookmark() { bookmark_ = 0; }

  void NextBookmark() { ++bookmark_; }

  void PreviousBookmark() { ++bookmark_; }

  std::size_t size() const { return stack_.size(); }

  std::size_t empty() const { return stack_.empty(); }

  bool is_open() const 
  {
    return stack_.is_open();
  }

 private:
  VariantStack history_;
  bookmark_type bookmark_;

  stack_type stack_;
   
  void RevertBookmark() {
   
    HistoryBookmark book;
    history_.Top(book);
    bookmark_ = book.bookmark;
  }

  void RevertSwap() {
    HistorySwap swap;
    history_.Top(swap);
    stack_.Swap(swap.i, swap.j);
    
  }

  void RevertPop() {
    HistoryPop pop;
    history_.Top(pop);
    stack_.Push(pop.data);
  }

  void RevertPush() {
    HistoryPush push;
    history_.Top(push);
    stack_.Pop();
  }

  void RevertSet() {
    HistorySet set;
    history_.Top(set);
    stack_.Set(set.i, set.data);
  }
  
  void RevertHeader() {
    HistoryHeader header;
    history_.Top(header);
    
    header_type h = stack_.header_extra();
    std::cout << " " << h.header << " > " << header.data ;
    
    h.header = header.data;
    stack_.SetExtraHeader(h);
  }  
};


}
}

#endif
