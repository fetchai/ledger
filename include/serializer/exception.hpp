#ifndef SERIALIZER_EXCEPTION_HPP
#define SERIALIZER_EXCEPTION_HPP

#include <exception>
#include <string>
namespace fetch {
namespace serializers {

namespace error {
typedef uint64_t error_type;
error_type const TYPE_ERROR = 0;
};

class SerializableException : public std::exception {
 public:
  SerializableException()
      : error_code_(error::TYPE_ERROR), explanation_("unknown") {}

  SerializableException(std::string explanation)
      : error_code_(error::TYPE_ERROR), explanation_(explanation) {}

  SerializableException(error::error_type error_code, std::string explanation)
      : error_code_(error_code), explanation_(explanation) {}

  virtual ~SerializableException() {}

  char const* what() const noexcept override { return explanation_.c_str(); }
  uint64_t error_code() const { return error_code_; }
  std::string explanation() const { return explanation_; }

 private:
  uint64_t error_code_;
  std::string explanation_;
};
};
};

#endif
