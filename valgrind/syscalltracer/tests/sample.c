/*
 * simple test
 */

#include <stdio.h>
#include "function_tracing.h"

int main(void)
{
  START_FUNCTION_TRACING;
  printf("Hello World\n");
  STOP_FUNCTION_TRACING;
  return 0;
}
