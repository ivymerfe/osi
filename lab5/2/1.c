#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int global_var = 1;

void show_maps() {
  return;
  int pid = getpid();
  char command[512];
  snprintf(command, sizeof(command),
           "kitty sh -c 'watch -n 1 cat /proc/%d/maps' > /dev/null 2>&1 &",
           pid);
  system(command);
}

uintptr_t get_page(uintptr_t virtual_addr) {
  int fd = open("/proc/self/pagemap", O_RDONLY);
  if (fd < 0) {
    perror("open pagemap");
    return 0;
  }

  uint64_t value;
  long page_size = sysconf(_SC_PAGESIZE);
  off_t offset = (virtual_addr / page_size) * sizeof(value);

  if (pread(fd, &value, sizeof(value), offset) != sizeof(value)) {
    close(fd);
    return 0;
  }
  close(fd);

  if (!(value & (1ULL << 63))) {
    return 0;
  }
  uint64_t pfn = value & ((1ULL << 55) - 1);
  return pfn;
}

void print_ptrs(int* g, int* l) {
  printf("Virt: G=%p, L=%p\n", (void*)g, (void*)l);

  uintptr_t phys_g = get_page((uintptr_t)g);
  uintptr_t phys_l = get_page((uintptr_t)l);

  printf("Phys: G=0x%lx, L=0x%lx\n", phys_g, phys_l);
  printf("Val : G=%d, L=%d\n", *g, *l);
}

int main() {
  int local_var = 2;

  printf("PID: %d\n", getpid());
  show_maps();
  print_ptrs(&global_var, &local_var);

  pid_t pid = fork();

  if (pid == 0) {
    printf("Child:\n");
    printf("pid: %d, ppid: %d\n", getpid(), getppid());
    show_maps();
    print_ptrs(&global_var, &local_var);

    sleep(1);
    global_var = 1337;
    local_var = 666;
    printf("Child changed:\n");
    print_ptrs(&global_var, &local_var);

    printf("Exiting with code 5...\n");
    exit(5);
  } else {
    printf("Parent:\n");
    printf("Child PID: %d\n", pid);
    print_ptrs(&global_var, &local_var);
    sleep(2);
    printf("Parent:\n");
    print_ptrs(&global_var, &local_var);

    sleep(1000);
    int status;
    // wait(&status);

    printf("Child done:\n");
    if (WIFEXITED(status)) {
      printf("Exit code: %d\n", WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
      printf("Terminated by signal: %d\n", WTERMSIG(status));
    }
  }

  return 0;
}
