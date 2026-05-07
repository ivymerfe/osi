#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKET_PATH "/tmp/govno.sock"

int main(void) {
  int fd = socket(AF_UNIX, SOCK_STREAM, 0);

  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

  connect(fd, (struct sockaddr*)&addr, sizeof(addr));

  printf("Connected\n");
  char buf[1024];
  while (fgets(buf, sizeof(buf), stdin) != NULL) {
    write(fd, buf, strlen(buf));

    size_t n = read(fd, buf, sizeof(buf) - 1);
    if (n <= 0)
      break;
    buf[n] = '\0';
    printf("Echo:  %s", buf);
  }

  close(fd);
  return 0;
}
