#ifndef FUNCTION_TRACING_H
#define FUNCTION_TRACING_H

#include "valgrind.h"

enum function_tracing_reqs {
  USERREQ_START_FUNCTION_TRACING = VG_USERREQ_TOOL_BASE('S', 'T'),
  USERREQ_STOP_FUNCTION_TRACING
};

#define START_FUNCTION_TRACING \
  { \
    int res; \
    VALGRIND_DO_CLIENT_REQUEST(res, 0, \
        USERREQ_START_FUNCTION_TRACING, \
        0, 0, 0, 0, 0); \
  }

#define STOP_FUNCTION_TRACING \
  { \
    int res; \
    VALGRIND_DO_CLIENT_REQUEST(res, 0, \
        USERREQ_STOP_FUNCTION_TRACING, \
        0, 0, 0, 0, 0); \
  }

#endif
