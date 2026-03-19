#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int parse_cmd(char* cmd) {
  if (!strcmp(cmd, "mkdir"))
    return 1;
  else if (!strcmp(cmd, "ls"))
    return 2;
  else if (!strcmp(cmd, "rmdir"))
    return 3;
  else if (!strcmp(cmd, "mkfile"))
    return 4;
  else if (!strcmp(cmd, "read"))
    return 5;
  else if (!strcmp(cmd, "rm"))
    return 6;
  else if (!strcmp(cmd, "symlink"))
    return 7;
  else if (!strcmp(cmd, "readlink"))
    return 8;
  else if (!strcmp(cmd, "linkdata"))
    return 9;
  else if (!strcmp(cmd, "rmlink"))
    return 10;
  else if (!strcmp(cmd, "hard"))
    return 11;
  else if (!strcmp(cmd, "rmhard"))
    return 12;
  else if (!strcmp(cmd, "stat"))
    return 13;
  else if (!strcmp(cmd, "chmod"))
    return 14;
  return -1;
}

int main(int argc, char* argv[]) {
  char* cmd = strrchr(argv[0], '/');
  cmd = cmd ? cmd + 1 : argv[0];
  switch (parse_cmd(cmd)) {
    case 1:
      mkdir(argv[1], 0755);
      break;
    case 2: {
      DIR* d = opendir(argv[1]);
      struct dirent* e;
      while ((e = readdir(d)))
        printf("%s\n", e->d_name);
      closedir(d);
      break;
    }
    case 3:
      rmdir(argv[1]);
      break;
    case 4: {
      int fd = open(argv[1], O_CREAT | O_WRONLY);
      close(fd);
      break;
    }
    case 5: {
      char buf[256];
      int fd = open(argv[1], O_RDONLY);
      int n;
      while ((n = read(fd, buf, sizeof(buf))) > 0)
        write(1, buf, n);
      close(fd);
      break;
    }
    case 6:
      unlink(argv[1]);
      break;
    case 7:
      symlink(argv[1], argv[2]);
      break;
    case 8: {
      char buf[256];
      int n = readlink(argv[1], buf, sizeof(buf) - 1);
      buf[n] = 0;
      printf("%s\n", buf);
      break;
    }
    case 9: {
      char buf[256];
      int n = readlink(argv[1], buf, sizeof(buf) - 1);
      buf[n] = 0;
      int fd = open(buf, O_RDONLY);
      while ((n = read(fd, buf, sizeof(buf))) > 0)
        write(1, buf, n);
      close(fd);
      break;
    }
    case 10:
      unlink(argv[1]);
      break;
    case 11:
      link(argv[1], argv[2]);
      break;
    case 12:
      unlink(argv[1]);
      break;
    case 13: {
      struct stat st;
      stat(argv[1], &st);
      printf("perms: %o\n", st.st_mode & 0777);
      printf("hard: %ld\n", st.st_nlink);
      break;
    }
    case 14:
      chmod(argv[1], strtol(argv[2], NULL, 8));
      break;
    default:
      fprintf(stderr, "bad cmd: %s\n", cmd);
      return 1;
  }
  return 0;
}
