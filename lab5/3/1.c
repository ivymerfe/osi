#define _GNU_SOURCE
#include <err.h>
#include <fcntl.h>
#include <sched.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <unistd.h>

#define STACK_SIZE (1024)
#define RECURSION_DEPTH 10

static void recurse(char depth) {
  char msg[] = "hello world";

  if (depth > 0) {
    recurse(depth - 1);
  }
}

static int child_entry(void* arg) {
  volatile void* a = child_entry;
  volatile void* b = recurse;
  recurse(RECURSION_DEPTH);
  return 0;
}

int main(void) {
  const char* stack_file = "./clone_stack.bin";
  int fd = open(stack_file, O_RDWR | O_CREAT | O_TRUNC, 0600);
  ftruncate(fd, STACK_SIZE);
  void* stack =
      mmap(NULL, STACK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (stack == MAP_FAILED) {
    close(fd);
    return 1;
  }

  void* stack_top = (char*)stack + STACK_SIZE;
  pid_t pid = clone(child_entry, stack_top, SIGCHLD, NULL);
  waitpid(pid, NULL, 0);
  msync(stack, STACK_SIZE, MS_SYNC);
  munmap(stack, STACK_SIZE);
  close(fd);

  return 0;
}
