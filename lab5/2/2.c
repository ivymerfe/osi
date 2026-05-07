#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

int main() {
  printf("Outer parent: %d\n", getpid());

  pid_t pid1 = fork();
  if (pid1 == 0) {
    printf("Inner parent: %d\n", getpid());

    pid_t pid2 = fork();
    if (pid2 == 0) {
      printf("Child: %d\n", getpid());
      sleep(2);
    } else {
      sleep(10);
    }
  } else {
    sleep(200);
  }
  return 0;
}
