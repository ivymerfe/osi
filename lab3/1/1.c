#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 4096

void reverse_string(char* str, size_t len) {
  for (size_t i = 0; i < len / 2; i++) {
    char tmp = str[i];
    str[i] = str[len - 1 - i];
    str[len - 1 - i] = tmp;
  }
}

int copy_rev(const char* src, const char* dst, mode_t mode) {
  int in_fd = open(src, O_RDONLY);
  if (in_fd < 0) return 0;
  
  int out_fd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, mode & 0777);
  if (out_fd < 0) {
    close(in_fd);
    return 0;
  }
  
  off_t size = lseek(in_fd, 0, SEEK_END);
  if (size < 0) {
    close(in_fd);
    close(out_fd);
    return 0;
  }
  
  char buf[BUFFER_SIZE];
  for (off_t left = size; left > 0;) {
    size_t chunk = left > (off_t)BUFFER_SIZE ? BUFFER_SIZE : (size_t)left;
    off_t offset = left - (off_t)chunk;
    
    ssize_t n = pread(in_fd, buf, chunk, offset);
    if (n < 0) {
      if (errno == EINTR) continue;
      close(in_fd);
      close(out_fd);
      return 0;
    }
    
    reverse_string(buf, n);
    if (write(out_fd, buf, n) < n) {
      close(in_fd);
      close(out_fd);
      return 0;
    }
    left -= n;
  }
  close(in_fd);
  close(out_fd);
  return 1;
}

int main(int argc, char** argv) {
  if (argc != 2) return 1;
  
  const char* source = argv[1];
  struct stat dir_st;
  if (stat(source, &dir_st) < 0 || !S_ISDIR(dir_st.st_mode)) {
    fprintf(stderr, "bad dir: %s\n", source);
    return 2;
  }
  
  char* source_copy = strdup(source);
  char* last_slash = strrchr(source_copy, '/');
  char parent[PATH_MAX], base[PATH_MAX];
  
  if (!last_slash) {
    strcpy(parent, ".");
    strcpy(base, source_copy);
  } else {
    *last_slash = '\0';
    strcpy(parent, source_copy);
    strcpy(base, last_slash + 1);
  }
  free(source_copy);
  
  reverse_string(base, strlen(base));
  char target[PATH_MAX];
  snprintf(target, PATH_MAX, "%s/%s", parent, base);
  
  if (mkdir(target, dir_st.st_mode & 0777) < 0) {
    fprintf(stderr, "%s\n", strerror(errno));
    return 3;
  }
  
  DIR* dir = opendir(source);
  if (!dir) {
    fprintf(stderr, "%s\n", strerror(errno));
    return 1;
  }
  
  struct dirent* entry;
  while ((entry = readdir(dir)) != NULL) {
    const char* name = entry->d_name;
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;
    
    char src_file[PATH_MAX];
    snprintf(src_file, PATH_MAX, "%s/%s", source, name);
    
    struct stat st;
    if (stat(src_file, &st) < 0 || !S_ISREG(st.st_mode)) continue;
    
    char rev_name[PATH_MAX];
    strcpy(rev_name, name);
    reverse_string(rev_name, strlen(rev_name));
    
    char dst_file[PATH_MAX];
    snprintf(dst_file, PATH_MAX, "%s/%s", target, rev_name);
    
    if (!copy_rev(src_file, dst_file, st.st_mode)) {
      fprintf(stderr, "%s\n", strerror(errno));
    }
  }
  closedir(dir);
  return 0;
}
