#ifndef BYTE_ARRAY_TOKENIZER_TOKENIZER_HPP
#define BYTE_ARRAY_TOKENIZER_TOKENIZER_HPP

#include "byte_array/referenced_byte_array.hpp"
#include "byte_array/tokenizer/token.hpp"
#include "assert.hpp"
#include <functional>
#include <map>
#include <vector>

namespace fetch {
namespace byte_array {

class Tokenizer : public std::vector<Token> {
 public:
  typedef ConstByteArray byte_array_type;
  typedef std::function<int(byte_array_type const &, uint64_t &)>
      consumer_function_type;

  typedef std::function<int(byte_array_type const &, uint64_t const &, int const &)>
      indexer_function_type;  

  void SetConsumerIndexer(indexer_function_type function) 
  {
    indexer_ = function;
    
  }
      
  std::size_t AddConsumer(consumer_function_type function) {

    std::size_t ret = consumers_.size();
    consumers_.push_back(function);
    return ret;
    
  }

  bool Parse(byte_array_type const& contents, bool clear = true) {
    uint64_t pos = 0;
    uint64_t line = 0;
    uint64_t char_index = 0;
    byte_array_type::container_type const *str = contents.pointer();
    if(clear) this->clear();
    
    // Counting tokens
    if(contents.size() > 100000) { // TODO: Optimise this parameter
      
      std::size_t n = 0;
      while (pos < contents.size()) {
        uint64_t oldpos = pos;
        int token_type = 0;

        if(indexer_) {
          int index = 0, prev_index = -1 ;
          bool check = true;        
          while(check) {
            index = indexer_(contents, pos, prev_index);
            
            auto &c = consumers_[index];
            
            pos = oldpos;
            token_type = c(contents, pos);
            if (token_type > -1) break;
            
            check = (index != prev_index );
            prev_index = index;
          }
        }
        else
        {
          for (auto &c : consumers_) {
            pos = oldpos;
            token_type = c(contents, pos);
            if (token_type > -1) break;
          }          
        }
        if (pos == oldpos) {
          TODO_FAIL( "Unable to parse char on ", pos, "  '", str[pos], "'", ", '", contents[pos], "'" );
        }        
        ++n;          
      }
      

      this->reserve(n);
    }
    
    // Extracting tokens
    pos = 0;    
    while (pos < contents.size()) {
      uint64_t oldpos = pos;
      int64_t token_type = 0;

      if(indexer_) {
        int index = 0, prev_index = -1 ;
        bool check = true;        
        while(check) {
          index = indexer_(contents, pos, prev_index);
          
          auto &c = consumers_[index];
          
          pos = oldpos;
          token_type = c(contents, pos);
          if (token_type > -1) break;

          check = (index != prev_index );
          prev_index = index;
        }
      }
      else
      {
        for (auto &c : consumers_) {
          pos = oldpos;
          token_type = c(contents, pos);
          if (token_type > -1) break;
        }
      }
      
      if (pos == oldpos) {
        TODO_FAIL( "Unable to parse char on ", pos, "  '", str[pos], "'", ", '", contents[pos], "'" );
      }
      
      this->emplace_back(contents, oldpos, pos - oldpos);
      Token &tok = this->back();

      tok.SetLine(line);
      tok.SetChar(char_index);
      tok.SetType(token_type);

      for (uint64_t i = oldpos; i < pos; ++i) {
        ++char_index;
        if (str[i] == '\n') {
          ++line;
          char_index = 0;
        }
      }
    }

    return true;
  }

 private:
  std::vector<consumer_function_type> consumers_;
  indexer_function_type indexer_;
  
};
};
};

#endif
