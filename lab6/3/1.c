#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKET_PATH "/tmp/govno.sock"
#define MAX_EVENTS 128
#define BUF_SIZE 1024
#define MAX_CLIENTS 1024

typedef struct {
  int fd;
  int id;
  char write_buf[BUF_SIZE];
  int write_len;
} client_t;

client_t* clients[MAX_CLIENTS];
int next_id = 1;

int set_nonblocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags < 0)
    return -1;
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void epoll_add(int epfd, int fd, uint32_t events) {
  struct epoll_event ev = {.events = events, .data.fd = fd};
  if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) < 0)
    perror("epoll_ctl ADD");
}

void epoll_mod(int epfd, int fd, uint32_t events) {
  struct epoll_event ev = {.events = events, .data.fd = fd};
  if (epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) < 0)
    perror("epoll_ctl MOD");
}

void client_disconnect(int epfd, int fd) {
  printf("Client #%d (fd=%d) disconnected.\n",
         clients[fd] ? clients[fd]->id : -1, fd);

  epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
  close(fd);

  if (clients[fd]) {
    free(clients[fd]);
    clients[fd] = NULL;
  }
}

void handle_accept(int epfd, int server_fd) {
  while (1) {
    int client_fd = accept(server_fd, NULL, NULL);

    if (client_fd < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        break;
      perror("accept");
      break;
    }
    set_nonblocking(client_fd);

    client_t* c = calloc(1, sizeof(client_t));
    c->fd = client_fd;
    c->id = next_id++;
    clients[client_fd] = c;

    epoll_add(epfd, client_fd, EPOLLIN);

    printf("Client #%d connected (fd=%d)\n", c->id, client_fd);
  }
}

void handle_read(int epfd, int fd) {
  client_t* client = clients[fd];
  if (!client)
    return;

  char buf[BUF_SIZE];
  ssize_t n = read(fd, buf, sizeof(buf) - 1);

  if (n <= 0) {
    if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
      return;
    client_disconnect(epfd, fd);
    return;
  }

  buf[n] = '\0';
  printf("Client %d: %s\n", client->id, buf);

  memcpy(client->write_buf, buf, n);
  client->write_len = n;

  ssize_t written = write(fd, client->write_buf, client->write_len);
  if (written < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
    written = 0;
  }

  if (written < client->write_len) {
    memmove(client->write_buf, client->write_buf + written,
            client->write_len - written);
    client->write_len -= written;
    epoll_mod(epfd, fd, EPOLLIN | EPOLLOUT);
  } else {
    client->write_len = 0;
  }
}

void handle_write(int epfd, int fd) {
  client_t* client = clients[fd];
  if (!client || client->write_len == 0) {
    return;
  }
  ssize_t written = write(fd, client->write_buf, client->write_len);
  if (written < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK)
      return;
  }
  client->write_len -= written;
  if (client->write_len == 0) {
    epoll_mod(epfd, fd, EPOLLIN);
  } else {
    memmove(client->write_buf, client->write_buf + written,
            client->write_len - written);
  }
}

int main(void) {
  int epfd = epoll_create1(0);
  int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);

  set_nonblocking(server_fd);
  unlink(SOCKET_PATH);

  struct sockaddr_un addr = {.sun_family = AF_UNIX};
  strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

  bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
  listen(server_fd, 128);

  epoll_add(epfd, server_fd, EPOLLIN);

  printf("Server listening on %s\n", SOCKET_PATH);

  struct epoll_event events[MAX_EVENTS];
  while (1) {
    int nready = epoll_wait(epfd, events, MAX_EVENTS, -1);
    if (nready < 0) {
      if (errno == EINTR)
        continue;
      perror("epoll_wait");
      break;
    }

    for (int i = 0; i < nready; i++) {
      int fd = events[i].data.fd;
      uint32_t ev = events[i].events;

      if (ev & (EPOLLERR | EPOLLHUP)) {
        if (fd != server_fd)
          client_disconnect(epfd, fd);
        continue;
      }

      if (fd == server_fd) {
        handle_accept(epfd, server_fd);
      } else {
        if (ev & EPOLLIN)
          handle_read(epfd, fd);
        if (ev & EPOLLOUT)
          handle_write(epfd, fd);
      }
    }
  }

  close(epfd);
  close(server_fd);
  unlink(SOCKET_PATH);
  return 0;
}
