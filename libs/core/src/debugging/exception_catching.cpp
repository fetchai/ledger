#include "debugging/exception_catching.hpp"

#include "core/logger.hpp"

// TODO(EJF): ???
static constexpr char const *LOGGING_NAME = "???";

namespace fetch {
namespace debugging {

  void withExceptionCatching(const char *fn, unsigned int ln, std::function<void (void)> func)
  {
    try
    {
      func();
    }
    catch(std::exception &ex)
    {
      FETCH_LOG_ERROR(LOGGING_NAME,"While at ", fn, ":", ln, " - ", ex.what());
      throw;
    }
  }

} // namespace debugging
} // namespace fetch
