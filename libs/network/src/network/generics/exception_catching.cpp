#include "network/generics/exception_catching.hpp"

#include "core/logger.hpp"

namespace fetch
{
namespace debugging
{

  void withExceptionCatching(const char *fn, unsigned int ln, std::function<void (void)> func)
  {
    try
    {
      func();
    }
    catch(std::exception &ex)
    {
      fetch::logger.Error("While at ", fn, ":", ln, " - ", ex.what());
      throw;
    }
  }




}
}
