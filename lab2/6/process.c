#include <stdio.h>
#include <sys/ptrace.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>

int main() {
  int pid = fork();
  if (pid == 0) {
    ptrace(PTRACE_TRACEME, 0, NULL, NULL);

    FILE* file = fopen("hello.txt", "a");
    fwrite("hello", 5, 1, file);
    fclose(file);
    return 0;
  }
  int status = -1;
  ptrace(PTRACE_ATTACH, pid, NULL, NULL);
  waitpid(pid, &status, 0);

  while (WIFSTOPPED(status)) {
    struct user_regs_struct regs;

    int ret = ptrace(PTRACE_GETREGS, pid, 0, &regs);
    if (ret != 0) {
      break;
    }
    printf("Syscall: %llu, %llu, %llu, %llu\n", regs.orig_rax, regs.rdi, regs.rsi, regs.rdx);
    ptrace(PTRACE_SYSCALL, pid);
    waitpid(pid, &status, 0);
  }
  printf("a\n");
  return 0;
}
