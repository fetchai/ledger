#pragma once
#include "core/logger.hpp"
#include <exception>
#include <string>
#include <utility>

namespace fetch {
namespace swarm {

class SwarmException : public std::exception
{
public:
  SwarmException(std::string explanation) : explanation_(std::move(explanation))
  {
    LOG_STACK_TRACE_POINT;
    LOG_SET_CONTEXT_VARIABLE(stack_trace_);
  }

  virtual ~SwarmException() { LOG_STACK_TRACE_POINT; }

  char const *what() const noexcept override { return explanation_.c_str(); }
  uint64_t    error_code() const { return 1; }
  std::string explanation() const { return explanation_; }

  void StackTrace() const { LOG_PRINT_STACK_TRACE(stack_trace_, "Trace at time of exception"); }

private:
  std::string explanation_;

  LOG_CONTEXT_VARIABLE(stack_trace_)
};

}  // namespace swarm
}  // namespace fetch
