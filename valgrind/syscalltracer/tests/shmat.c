/*
 * test with shared memory
 */

#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>

#include "function_tracing.h"

int main(void)
{
  key_t key = ftok("shmat.c", 'R');
  if (key == -1) {
    perror("ftok");
    return 1;
  }

  START_FUNCTION_TRACING;
  int shmid = shmget(key, 4096, 0644 | IPC_CREAT);
  if (shmid == -1) {
    fprintf(stderr, "shmget failed\n");
    return 1;
  }

  void *shm = shmat(shmid, NULL, 0);
  if (shm == (void *) -1) {
    perror("shmat");
    return 1;
  }

  if (shmdt(shm) == -1) {
    perror("shmdt");
    return 1;
  }
  STOP_FUNCTION_TRACING;

  return 0;
}
