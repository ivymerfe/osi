#include <stdio.h>
#include <unistd.h>

int main() {
	uid_t real_uid = getuid();
	uid_t eff_uid = geteuid();
	printf("Real UID: %d\n", real_uid);
	printf("Effective UID: %d\n", eff_uid);

  FILE *file = fopen("secret", "r");
  if (file == NULL) {
    perror("cannot open");
    return 1;
  }
  char buf[4096];
  size_t n = fread(buf, 1, sizeof(buf), file);
  if (ferror(file)) {
    perror("failed to read");
    fclose(file);
    return 1;
  }
  buf[n] = 0;
  printf("Content: %s", buf);
  fclose(file);
  return 0;
}
