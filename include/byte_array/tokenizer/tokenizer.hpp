#ifndef BYTE_ARRAY_TOKENIZER_TOKENIZER_HPP
#define BYTE_ARRAY_TOKENIZER_TOKENIZER_HPP

#include <byte_array/referenced_byte_array.hpp>
#include <byte_array/tokenizer/token.hpp>

#include <functional>
#include <map>
#include <vector>

namespace fetch {
namespace byte_array {

class Tokenizer : public std::vector<Token> {
 public:
  typedef ConstByteArray byte_array_type;
  typedef std::function<bool(byte_array_type const &, uint64_t &)>
      consumer_function_type;

 private:
  struct ConsumerFunction {
    consumer_function_type function;
    uint64_t type;
  };

  consumer_function_type Keyword(std::string keyword) {
    return [keyword](byte_array_type const &str, uint64_t &pos) -> bool {
      if (str.Match(keyword.c_str(), pos)) {
        pos += keyword.size();
        return true;
      }
      return false;
    };
  }

  struct TokenSpace {
    byte_array_type name;
    uint64_t outer_open;
    uint64_t inner_open;
    uint64_t close;
    uint64_t counter;
    byte_array_type parent;

    bool can_close;
  };

  void CreateTokenSpace(byte_array_type const &name,
                        uint64_t const &outer_open = uint64_t(-1),
                        uint64_t const &inner_open = uint64_t(-1),
                        uint64_t const &close = uint64_t(-1),
                        byte_array_type const &tokenspace = "") {
    TokenSpace space = {name,
                        outer_open,
                        inner_open,
                        close,
                        (outer_open == inner_open ? 1ull : 0ull),
                        tokenspace,
                        (outer_open == inner_open)};

    if (spaceselector_.find(tokenspace) == spaceselector_.end())
      spaceselector_[tokenspace] = std::vector<TokenSpace>();

    spaceselector_[tokenspace].push_back(space);
  }

 public:

  void CreateSubspace(byte_array_type const &name, uint64_t const &type1,
                      byte_array_type const &token1, uint64_t const &type2,
                      byte_array_type const &token2,
                      byte_array_type const &parent_space = "") {
    AddConsumer(type1, Keyword(token1), parent_space);
    AddConsumer(type1, Keyword(token1), name);
    AddConsumer(type2, Keyword(token2), name);
    CreateTokenSpace(name, type1, type1, type2, parent_space);
  }

  void AddConsumer(uint64_t const &token_type, consumer_function_type function,
                   byte_array_type const &tokenspace = "") {
    ConsumerFunction fnc;
    fnc.function = function;
    fnc.type = token_type;

    if (consumers_.find(tokenspace) == consumers_.end())
      consumers_[tokenspace] = std::vector<ConsumerFunction>();
    consumers_[tokenspace].push_back(fnc);
  }

  bool Parse(byte_array_type filename, byte_array_type const& contents) {
    uint64_t pos = 0;
    uint64_t line = 0;
    uint64_t char_index = 0;
    byte_array_type::container_type const *str = contents.pointer();

    std::vector<TokenSpace> tokenspaces;
    tokenspaces.push_back({""});

    while (pos < contents.size()) {
      uint64_t oldpos = pos;
      uint64_t token_type = 0;

      TokenSpace &space = tokenspaces.back();
      auto &cons = consumers_[space.name];
      for (auto &c : cons) {
        token_type = c.type;
        if (c.function(contents, pos)) break;
      }

      if (pos == oldpos) {
        TODO_FAIL( "Unable to parse char on ", pos, "  '", str[pos], "'", ", '", contents[pos], "'" );
      }

      if (space.inner_open == token_type) {
        ++space.counter;
        space.can_close = true;
      } else if (space.close == token_type) {
        --space.counter;
        space.can_close = true;
      }

      if (space.can_close && (space.counter == 0)) {
        tokenspaces.pop_back();
        if (tokenspaces.size() == 0) {
          TODO_FAIL("This is not suppose to happen" );

        }
      } else {
        if (spaceselector_.find(space.name) != spaceselector_.end()) {
          for (auto &s : spaceselector_[space.name])
            if (s.outer_open == token_type) {
              tokenspaces.push_back(s);
              break;
            }
        }
      }

      Token tok(contents, oldpos, pos - oldpos);

      tok.SetFilename(filename);
      tok.SetLine(line);
      tok.SetChar(char_index);
      tok.SetType(token_type);
      this->push_back(tok);
      for (uint64_t i = oldpos; i < pos; ++i) {
        ++char_index;
        if (str[i] == '\n') {
          ++line;
          char_index = 0;
          break;
        }
      }
    }

    return true;
  }

 private:
  std::map<byte_array_type, std::vector<ConsumerFunction> > consumers_;
  std::map<byte_array_type, std::vector<TokenSpace> > spaceselector_;
};
};
};

#endif
