#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int global_initialized = 10;
int global_notinit[1024 * 1024];
const int global_const = 1337;

long long makelocal() {
  int local = 133;
  return (long long)&local;
}

void printmake2() {
  printf("makelocal 2: %p\n", makelocal());
}

void heapfun() {
  void* bug = malloc(100);
  strcpy(bug, "hello world");
  write(1, bug, 100);
  write(1, "\n\n", 2);
  free(bug);
  write(1, bug, 100);
  write(1, "\n\n", 2);

  void* buf2 = malloc(100);
  strcpy(buf2, "goodbye world");
  write(1, buf2, 100);
  write(1, "\n\n", 2);
//   free(buf2 + 50);
  write(1, buf2   + 5, 100);
  write(1, "\n\n", 2);
}

extern char** environ;

void envshit() {
  for (char** env = environ; *env != NULL; env++) {
    printf("%s\n", *env);
    // if (strncmp(*env, "MIMI=", 5) == 0) {
    //   printf("%s\n", *env);
    // }
  }
  for (char** env = environ; *env != NULL; env++) {
    if (strncmp(*env, "MIMI=", 5) == 0) {
      *env = "MIMI=aa";
      break;
    }
  }
  printf("new =  %s\n", getenv("MIMI"));
}

int main() {
  int local;
  static int static_local;
  const int const_local;

  printf("local: %p\n", &local);
  printf("static_local: %p\n", &static_local);
  printf("const_local: %p\n", &const_local);
  printf("global_initialized: %p\n", &global_initialized);
  printf("global_notinit: %p\n", &global_notinit);
  printf("global_const: %p\n", &global_const);

  printf("makelocal 1: %p\n", makelocal());
  printmake2();

  heapfun();
  envshit();

  printf("my pid: %d\n", getpid());
  getchar();

  return 0;
}