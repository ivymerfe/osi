#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

int main() {
  printf("pid = %d\n", getpid());

  void* pointers[100];

  for (int i = 0; i < 100; i++) {
    pointers[i] = malloc(1024 * 1024);
    sleep(1);
  }
  for (int i = 0; i < 100; i++) {
    free(pointers[i]);
  }
  return 0;
}
