#include "core/logger.hpp"

// using Logger = fetch::log::Logger;

void breakpoint(void)
{
  FETCH_LOG_ERROR("breakpoint", "breakpoint function");
}
