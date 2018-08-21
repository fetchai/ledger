#pragma once

#include <functional>

namespace fetch
{
namespace debugging
{

  void withExceptionCatching(const char *fn, unsigned int ln, std::function<void (void)> func);

}
}

#define LOG_EX(FN,LN) debugging::withExceptionCatching(FN, LN, [=](){
#define END_LOG_EX });

