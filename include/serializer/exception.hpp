#ifndef SERIALIZER_EXCEPTION_HPP
#define SERIALIZER_EXCEPTION_HPP
#include"logger.hpp"
#include"byte_array/basic_byte_array.hpp"
#include <exception>
#include <string>
namespace fetch {
namespace serializers {

namespace error {
typedef uint64_t error_type;
error_type const TYPE_ERROR = 0;
}

class SerializableException : public std::exception {
 public:
  SerializableException()
      : error_code_(error::TYPE_ERROR), explanation_("unknown") {
    LOG_STACK_TRACE_POINT;

    LOG_SET_CONTEXT_VARIABLE(stack_trace_)
  }

  SerializableException(std::string explanation)
      : error_code_(error::TYPE_ERROR), explanation_(explanation) {
    LOG_STACK_TRACE_POINT;

    LOG_SET_CONTEXT_VARIABLE(stack_trace_)
  }

  SerializableException(byte_array::BasicByteArray const & explanation)
    : error_code_(error::TYPE_ERROR), explanation_(std::string(explanation)) {
    LOG_STACK_TRACE_POINT;

    LOG_SET_CONTEXT_VARIABLE(stack_trace_)
  }  

  SerializableException(error::error_type error_code, std::string explanation)
      : error_code_(error_code), explanation_(explanation) {
    LOG_STACK_TRACE_POINT;

    LOG_SET_CONTEXT_VARIABLE(stack_trace_)
  }

  SerializableException(error::error_type error_code, byte_array::BasicByteArray const & explanation)
    : error_code_(error_code), explanation_(std::string(explanation)) {
    LOG_STACK_TRACE_POINT;

    LOG_SET_CONTEXT_VARIABLE(stack_trace_)
  }  


  
  virtual ~SerializableException() {
    LOG_STACK_TRACE_POINT;    
  }

  char const* what() const noexcept override { return explanation_.c_str(); }
  uint64_t error_code() const { return error_code_; }
  std::string explanation() const { return explanation_; }

  void StackTrace() const
  {
    LOG_PRINT_STACK_TRACE(stack_trace_, "Trace at time of exception" );    
  }
  
 private:
  uint64_t error_code_;
  std::string explanation_;

  LOG_CONTEXT_VARIABLE(stack_trace_)
  
};
}
}

#endif
