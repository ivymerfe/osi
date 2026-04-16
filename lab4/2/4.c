#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

int main() {
  printf("pid = %d\n", getpid());

  void* addr = (void*)0x13371000;
  char* myy = mmap(addr, 10 * 4096, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

  strcpy(myy, "fuck you!");
  // mprotect(myy, 4096, PROT_WRITE);
  char a = (char)myy[1];
  printf("a = %c\n", a);
  // mprotect(myy, 4096, PROT_READ);
  strcpy(myy, "Noo!!");
  munmap(myy + 3 * 4096, 3 * 4096);
  char b = myy[3*4096 + 100];

  getchar();
}
