#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

void child_main(int pip[2]) {
  dup2(pip[0], STDIN_FILENO);

  char buf[256];
  while (fgets(buf, sizeof(buf), stdin) != NULL) {
    printf("Message: %s", buf);
  }
  printf("Child eof\n");
}

void parent_main(int pip[2]) {
  dup2(pip[1], STDOUT_FILENO);

  printf("Hi\n");
  printf("Bitch\n");
  fflush(stdout);

  close(STDOUT_FILENO);
}

int main(void) {
  int pip[2];
  pipe(pip);
  pid_t pid = fork();
  if (pid == 0) {
    child_main(pip);
    return 0;
  } else {
    parent_main(pip);
    waitpid(pid, NULL, 0);
  }
  return 0;
}
