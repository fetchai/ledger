#pragma once

enum ExitState
  {
    COMPLETE,
    DEFER,
    CANCELLED,
    ERRORED,
    RERUN
  };

extern const char *const exitStateNames[];
