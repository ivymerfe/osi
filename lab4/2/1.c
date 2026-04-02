#include <stdio.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
  printf("pid = %d\n", getpid());
  sleep(1);
  execl(argv[0], argv[0], NULL);
  printf("hello world\n");
}