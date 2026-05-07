#define _GNU_SOURCE
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

typedef struct {
  pid_t writer_pid;
  pid_t reader_pid;
} shared_pids_t;

shared_pids_t* shared_pids = NULL;

unsigned int* region = NULL;
size_t region_size = 0;

volatile sig_atomic_t sig_int = 0;
volatile sig_atomic_t sig_ready = 0;
volatile sig_atomic_t sig_die = 0;

void signal_handler(int sig) {
  switch (sig) {
    case SIGINT:
      sig_int = 1;
      break;
    case SIGUSR1:
      sig_ready = 1;
      break;
    case SIGUSR2:
      sig_die = 1;
      break;
  }
}

int writer_main() {
  signal(SIGINT, signal_handler);
  signal(SIGUSR1, signal_handler);
  signal(SIGUSR2, signal_handler);

  sigset_t mask, oldmask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGUSR1);
  sigprocmask(SIG_BLOCK, &mask, &oldmask);

  sig_ready = 0;
  while (!sig_ready && !sig_int) {
    sigsuspend(&oldmask);
  }

  pid_t reader_pid = shared_pids->reader_pid;
  size_t count = region_size / sizeof(unsigned int);
  unsigned int value = 0;
  while (!sig_int && !sig_die) {
    for (size_t i = 0; i < count; ++i) {
      region[i] = value++;
    }
    printf("Writer sent\n");
    kill(reader_pid, SIGUSR1);
    sig_ready = 0;
    while (!sig_ready && !sig_int && !sig_die) {
      sigsuspend(&oldmask);
    }
  }
  if (sig_int) {
    printf("Writer interrupted\n");
    kill(reader_pid, SIGUSR2);
  } else if (sig_die) {
    printf("Writer stopped because reader is killed\n");
  }

  return 0;
}

int reader_main() {
  signal(SIGINT, signal_handler);
  signal(SIGUSR1, signal_handler);
  signal(SIGUSR2, signal_handler);

  sigset_t mask, oldmask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGUSR1);
  sigprocmask(SIG_BLOCK, &mask, &oldmask);

  sig_ready = 0;
  while (!sig_ready && !sig_int) {
    sigsuspend(&oldmask);
  }

  pid_t writer_pid = shared_pids->writer_pid;
  size_t count = region_size / sizeof(unsigned int);
  unsigned int expected = 0;
  while (1) {
    sig_ready = 0;
    while (!sig_ready && !sig_int && !sig_die) {
      sigsuspend(&oldmask);
    }
    if (sig_int || sig_die) {
      break;
    }
    for (size_t i = 0; i < count; ++i) {
      unsigned int value = region[i];
      if (value != expected) {
        fprintf(stderr, "order failed at %zu: expected %u, got %u\n", i,
                expected, value);
        expected = value;
      }
      expected++;
    }
    printf("Reader received\n");
    kill(writer_pid, SIGUSR1);
  }
  if (sig_int) {
    printf("Reader interrupt\n");
    kill(writer_pid, SIGUSR2);
  } else if (sig_die) {
    printf("Reader stopped because writer is killed\n");
  }
  return 0;
}

int main(void) {
  region_size = 4096;
  region = mmap(NULL, region_size, PROT_READ | PROT_WRITE,
                MAP_SHARED | MAP_ANONYMOUS, -1, 0);

  shared_pids = mmap(NULL, sizeof(*shared_pids), PROT_READ | PROT_WRITE,
                     MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  shared_pids->writer_pid = 0;
  shared_pids->reader_pid = 0;

  pid_t writer_pid = fork();
  if (writer_pid == 0) {
    return writer_main();
  }

  pid_t reader_pid = fork();
  if (reader_pid == 0) {
    return reader_main();
  }

  shared_pids->writer_pid = writer_pid;
  shared_pids->reader_pid = reader_pid;

  printf("Writer PID: %d\n", writer_pid);
  printf("Reader PID: %d\n", reader_pid);

  usleep(7777);
  kill(writer_pid, SIGUSR1);
  kill(reader_pid, SIGUSR1);

  waitpid(writer_pid, NULL, 0);
  waitpid(reader_pid, NULL, 0);

  munmap(shared_pids, sizeof(*shared_pids));
  munmap(region, region_size);
  return 0;
}
