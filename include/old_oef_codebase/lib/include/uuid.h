#pragma once

#include <sstream>
#include <random>
#include <iomanip>      // std::setfill, std::setw
#include "serialize.h"

class Uuid {
 private:
  uint64_t _ab, _cd;
  explicit Uuid(uint64_t ab, uint64_t cd) : _ab{ab}, _cd{cd} {}
 public:
  explicit Uuid(const std::string &s) : _ab{0}, _cd{0} {
    char sep;
    uint64_t a,b,c,d,e;
    auto idx = s.find_first_of("-");
    if(idx != std::string::npos) {
      std::stringstream ss{s};
      if(ss >> std::hex >> a >> sep >> b >> sep >> c >> sep >> d >> sep >> e) {
        if(ss.eof()) {
          _ab = (a << 32) | (b << 16) | c;
          _cd = (d << 48) | e;
        }
      }
    }
  }
 public:
  static Uuid uuid4() {
    static std::random_device rd;
    static std::uniform_int_distribution<uint64_t> dist(0, (uint64_t)(~0));

    uint64_t ab = dist(rd);
    uint64_t cd = dist(rd);
    
    ab = (ab & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL;
    cd = (cd & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;
    
    return Uuid{ab, cd};
  }
  template <typename Archive>
	explicit Uuid(const Archive &ar) : Uuid{ar.getString("UUID")} {}
	template <typename Archive>
	void serialize(Archive &ar) const {
    const std::string s = to_string();
		ar.write("UUID", s);
	}
  size_t hash() const {
    return _ab ^ _cd;
  }
  bool operator==(const Uuid &other) {
    return _ab == other._ab && _cd == other._cd;
  }
  bool operator!=(const Uuid &other) {
    return !operator==(other);
  }
  bool operator<(const Uuid &other) {
    if(_ab < other._ab)
      return true;
    if(_ab > other._ab)
      return false;
    if(_cd < other._cd)
      return true;
    return false;
  }
  std::string to_string() const {
    std::stringstream ss;
    ss << std::hex << std::nouppercase << std::setfill('0');
    uint32_t a = (_ab >> 32);
    uint32_t b = (_ab & 0xFFFFFFFF);
    uint32_t c = (_cd >> 32);
    uint32_t d = (_cd & 0xFFFFFFFF);
    ss << std::setw(8) << (a) << '-';
    ss << std::setw(4) << (b >> 16) << '-';
    ss << std::setw(4) << (b & 0xFFFF) << '-';
    ss << std::setw(4) << (c >> 16 ) << '-';
    ss << std::setw(4) << (c & 0xFFFF);
    ss << std::setw(8) << d;
    return ss.str();
  }
};
