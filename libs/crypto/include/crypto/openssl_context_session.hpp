#pragma once

#include "crypto/openssl_context_detail.hpp"
#include "crypto/openssl_memory.hpp"

namespace fetch {
namespace crypto {
namespace openssl {
namespace context {

template <typename T, typename T_SessionPrimitive = detail::SessionPrimitive<T>,
          typename T_ContextSmartPtr = memory::ossl_shared_ptr<T>>
class Session
{
public:
  using type                   = T;
  using context_smart_ptr      = T_ContextSmartPtr;
  using session_primitive_type = T_SessionPrimitive;

private:
  context_smart_ptr context_;
  bool              is_started_;

public:
  explicit Session(context_smart_ptr context, const bool is_already_started = false)
      : context_(context), is_started_(is_already_started)
  {
    start();
  }

  explicit Session() : Session(context_smart_ptr(BN_CTX_new())) {}

  ~Session() { end(); }

  void start()
  {
    if (is_started_) return;

    session_primitive_type::start(context_.get());
    is_started_ = true;
  }

  void end()
  {
    if (!is_started_) return;

    is_started_ = false;
    session_primitive_type::end(context_.get());
  }

  context_smart_ptr context() const { return context_; }

  bool isStarted() const { return is_started_; }
};

}  // namespace context
}  // namespace openssl
}  // namespace crypto
}  // namespace fetch
