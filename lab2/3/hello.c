#include <unistd.h>
#include <sys/syscall.h>
#include <stdio.h>

int my_write(int fd, const char *buf, int count) {
    return syscall(SYS_write, fd, buf, count);
}

int main() {
    printf("%d\n", SYS_write);
    my_write(1, "Hello world\n", 12);
    return 0;
}
