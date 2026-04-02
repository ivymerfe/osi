
#include <unistd.h>
#include <stdio.h>

void rec_stack() {
    int garbage[1024];
    usleep(10000);
    rec_stack();
}

int main() {
    printf("pid = %d\n", getpid());
    sleep(1);
    rec_stack();
    return 0;
}
