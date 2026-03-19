#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

typedef enum {
  CMD_MKDIR = 1,
  CMD_LS = 2,
  CMD_RMDIR = 3,
  CMD_MKFILE = 4,
  CMD_READ = 5,
  CMD_RM = 6,
  CMD_SYMLINK = 7,
  CMD_READLINK = 8,
  CMD_LINKDATA = 9,
  CMD_RMLINK = 10,
  CMD_HARD = 11,
  CMD_RMHARD = 12,
  CMD_STAT = 13,
  CMD_CHMOD = 14,
  CMD_INVALID = -1
} Command;

typedef struct {
  const char* name;
  Command code;
} CommandDef;

CommandDef commands[] = {{"mkdir", CMD_MKDIR},       {"ls", CMD_LS},
                         {"rmdir", CMD_RMDIR},       {"mkfile", CMD_MKFILE},
                         {"read", CMD_READ},         {"rm", CMD_RM},
                         {"symlink", CMD_SYMLINK},   {"readlink", CMD_READLINK},
                         {"linkdata", CMD_LINKDATA}, {"rmlink", CMD_RMLINK},
                         {"hard", CMD_HARD},         {"rmhard", CMD_RMHARD},
                         {"stat", CMD_STAT},         {"chmod", CMD_CHMOD},
                         {NULL, CMD_INVALID}};
Command parse_cmd(const char* cmd) {
  for (int i = 0; commands[i].name; i++)
    if (!strcmp(cmd, commands[i].name))
      return commands[i].code;
  return CMD_INVALID;
}

void read_file(int fd) {
  char buf[4096];
  int n;
  while ((n = read(fd, buf, sizeof(buf))) > 0)
    write(1, buf, n);
  close(fd);
}

int main(int argc, char* argv[]) {
  char* cmd = strrchr(argv[0], '/');
  cmd = cmd ? cmd + 1 : argv[0];

  switch (parse_cmd(cmd)) {
    case CMD_MKDIR:
      mkdir(argv[1], 0755);
      break;
    case CMD_LS: {
      DIR* d = opendir(argv[1]);
      if (d) {
        struct dirent* e;
        while ((e = readdir(d)))
          printf("%s\n", e->d_name);
        closedir(d);
      }
      break;
    }
    case CMD_RMDIR:
      rmdir(argv[1]);
      break;
    case CMD_MKFILE: {
      int fd = open(argv[1], O_CREAT | O_WRONLY, 0644);
      if (fd >= 0)
        close(fd);
      break;
    }
    case CMD_READ: {
      int fd = open(argv[1], O_RDONLY);
      if (fd >= 0)
        read_file(fd);
      break;
    }
    case CMD_RM:
    case CMD_RMLINK:
    case CMD_RMHARD:
      unlink(argv[1]);
      break;
    case CMD_SYMLINK:
      symlink(argv[1], argv[2]);
      break;
    case CMD_READLINK: {
      char buf[256];
      int n = readlink(argv[1], buf, sizeof(buf) - 1);
      if (n >= 0) {
        buf[n] = 0;
        printf("%s\n", buf);
      }
      break;
    }
    case CMD_LINKDATA: {
      char buf[256];
      int n = readlink(argv[1], buf, sizeof(buf) - 1);
      if (n >= 0) {
        buf[n] = 0;
        int fd = open(buf, O_RDONLY);
        if (fd >= 0)
          read_file(fd);
      }
      break;
    }
    case CMD_HARD:
      link(argv[1], argv[2]);
      break;
    case CMD_STAT: {
      struct stat st;
      if (stat(argv[1], &st) == 0) {
        printf("perms: %o\n", st.st_mode & 0777);
        printf("hard: %ld\n", st.st_nlink);
      }
      break;
    }
    case CMD_CHMOD:
      chmod(argv[1], strtol(argv[2], NULL, 8));
      break;
    case CMD_INVALID:
      fprintf(stderr, "bad cmd: %s\n", cmd);
      return 1;
  }
  return 0;
}
