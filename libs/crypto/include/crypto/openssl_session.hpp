#ifndef CRYPTO_OPENSSL_CONTEXT_HPP
#define CRYPTO_OPENSSL_CONTEXT_HPP

#include "crypto/openssl_context_detail.hpp"
#include "crypto/openssl_memory_detail.hpp"

namespace fetch {
namespace crypto {
namespace openssl {
namespace context {

template <typename T,
        , typename T_SessionPrimitive = detail::SessionPrimitive<T>,
        , typename T_DeleterPrimitive = memory::detail::DeleterPrimitive<typename std::remove_const<T>::type>>
class Session
{
public:
    using Type = T;
    using NC_Type = typename std::remove_const<T>::type;

    using SessionPrimitive = T_SessionPrimitive;
    using DeleterPrimitive = T_DeleterPrimitive;

private:
    Type* _ptr;
    bool _isStarted;

public:
    explicit Session(Type* ptr, const bool is_already_started = false)
        : _ptr(ptr)
        , _isStarted(is_already_started)
    {
    }

    ~Session() {
        end();
        (*DeleterPrimitive::function)(const_cast<NC_Type*>(_ptr));
    }

    void start() {
        if (_isStarted)
            return;

        SessionPrimitive::start(_ptr);
        _isStarted = true;
    }

    void end() {
        if (!_isStarted)
            return;

        _isStarted = false;
        SessionPrimitive::stop(_ptr);
    }

    bool isStarted() const {
        return _isStarted;
    }

    NC_Type* operator-> () {
        return _ptr;
    }

    const NC_Type* operator-> () const {
        return _ptr;
    }

    NC_Type& operator * () {
        return *_ptr;
    }

    const NC_Type& operator * () const {
        return *_ptr;
    }

    NC_Type* operator T* () {
        return _ptr;
    }

    const NC_Type* operator const T* () const {
        return _ptr;
    }
};

} //* context namespace
} //* openssl namespace
} //* crypto namespace
} //* fetch namespace

#endif //CRYPTO_OPENSSL_CONTEXT_HPP

