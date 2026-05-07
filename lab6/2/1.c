#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

volatile sig_atomic_t sig_stop = 0;
void int_handler(int sig) {
  sig_stop = 1;
}

int writer_main(int pipe_out) {
  unsigned int value = 0;
  while (!sig_stop) {
    if (write(pipe_out, &value, sizeof(value)) <= 0)
      break;
    printf("Writer sent: %u\n", value);
    value++;
  }
  return 0;
}

int reader_main(int pipe_in) {
  unsigned int received;
  unsigned int expected = 0;
  while (!sig_stop) {
    if (read(pipe_in, &received, sizeof(received)) <= 0)
      break;

    if (received != expected) {
      fprintf(stderr, "Error: expected %u, got %u\n", expected, received);
      expected = received;
    }
    printf("Reader received: %u\n", received);
    expected++;
  }
  return 0;
}

int main() {
  int pip[2];
  pipe(pip);
  signal(SIGINT, int_handler);

  pid_t writer_pid = fork();
  if (writer_pid == 0) {
    return writer_main(pip[1]);
  }

  pid_t reader_pid = fork();
  if (reader_pid == 0) {
    return reader_main(pip[0]);
  }
  printf("Writer PID: %d\n", writer_pid);
  printf("Reader PID: %d\n", reader_pid);

  waitpid(writer_pid, NULL, 0);
  waitpid(reader_pid, NULL, 0);

  return 0;
}
