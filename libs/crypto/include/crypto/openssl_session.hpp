#ifndef CRYPTO_OPENSSL_CONTEXT_HPP
#define CRYPTO_OPENSSL_CONTEXT_HPP

#include "crypto/openssl_context_detail.hpp"
#include "crypto/openssl_memory.hpp"

namespace fetch {
namespace crypto {
namespace openssl {
namespace context {

template <typename T,
        , typename T_SessionPrimitive = detail::SessionPrimitive<T>,
class Session
{
public:
    using Type = T;
    using NC_Type = typename std::remove_const<T>::type;
    using ContextSmartPtr = ossl_shared_ptr<T>;
    using SessionPrimitive = T_SessionPrimitive;

private:
    ossl_shared_ptr<T> _context;
    bool _isStarted;

public:
    explicit Session(ContextSmartPtr context, const bool is_already_started = false)
        : _context(context)
        , _isStarted(is_already_started)
    {
    }

    ~Session() {
        end();
        (*DeleterPrimitive::function)(const_cast<NC_Type*>(_context.get()));
    }

    void start() {
        if (_isStarted)
            return;

        SessionPrimitive::start(_context.get());
        _isStarted = true;
    }

    void end() {
        if (!_isStarted)
            return;

        _isStarted = false;
        SessionPrimitive::stop(_context.get());
    }

    ContextSmartPtr context() const {
        return _context;
    }

    bool isStarted() const {
        return _isStarted;
    }
};

} //* context namespace
} //* openssl namespace
} //* crypto namespace
} //* fetch namespace

#endif //CRYPTO_OPENSSL_CONTEXT_HPP

