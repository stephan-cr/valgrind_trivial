/*
 * remap test case
 */

#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char **argv)
{
  char map[] = "map\n";
  char remap[] = "remap\n";

  if (argc < 2) {
    fprintf(stderr, "usage: %s <file>\n", argv[0]);
    return -1;
  }

  int fd = open(argv[1], O_RDWR);
  if (fd < 0) {
    perror("open");
    return 1;
  }

  write(1, map, sizeof(map) - 1);
  char *m = mmap(NULL, 0x2000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (m == MAP_FAILED) {
    perror("mmap");
    close(fd);
    return 2;
  }

  write(1, "write1\n", 7);
  m[0] = 23;

  write(1, remap, sizeof(remap) - 1);
  m = mremap(m, 0x2000, 0x4000, MREMAP_MAYMOVE);
  if (m == MAP_FAILED) {
    perror("mremap");
    return 3;
  }

  write(1, "write2\n", 7);
  m[1] = 23;
  if (munmap(m, 0x4000) == -1) {
    perror("munmap");
    close(fd);
    return 4;
  }

  close(fd);
  return 0;
}
