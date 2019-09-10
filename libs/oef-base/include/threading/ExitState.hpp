#pragma once

enum ExitState
  {
    COMPLETE,
    DEFER,
    CANCELLED,
    ERRORED
  };

extern const char *const exitStateNames[];
