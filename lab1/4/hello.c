#include <dlfcn.h>
#include <stdio.h>

int main() {
    void* h = dlopen("./libdynamic.so", RTLD_NOW);
    if (!h) {
        fprintf(stderr, "open err: %s\n", dlerror());
        return 1;
    }
    void (*hello)(void) = dlsym(h, "hello_from_dynamic_lib");
    if (!hello) {
        fprintf(stderr, "no symbol: %s\n", dlerror());
        return 2;
    }
    hello();
    return 0;
}
