#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main() {
  size_t region_size = 4096;
  unsigned int* region = mmap(NULL, region_size, PROT_READ | PROT_WRITE,
                              MAP_SHARED | MAP_ANONYMOUS, -1, 0);

  size_t count = region_size / sizeof(unsigned int);

  pid_t writer_pid = fork();
  if (writer_pid == 0) {
    unsigned int value = 0;
    while (1) {
      for (size_t i = 0; i < count; ++i) {
        region[i] = value++;
      }
    }
    return 0;
  }

  pid_t reader_pid = fork();
  if (reader_pid == 0) {
    unsigned int expected = 0;
    while (1) {
      for (size_t i = 0; i < count; ++i) {
        unsigned int value = region[i];
        if (value != expected) {
          fprintf(stderr, "order failed at %zu: expected %u, got %u\n",
                  i, expected, value);
          expected = value;
        }
        expected++;
      }
    }
    return 0;
  }

  waitpid(writer_pid, NULL, 0);
  waitpid(reader_pid, NULL, 0);
  munmap(region, region_size);
  return 0;
}
