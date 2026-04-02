#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define MEM_SIZE (1000 * 4096)

void* mem;
void* mem_curr;

typedef struct block {
  size_t size;
  struct block* next;
} block;

block* free_list = NULL;

void* my_malloc(int size) {
  // ищем блок в списке free
  block** p = &free_list;
  while (*p) {
    if ((*p)->size >= size) {
      block* b = *p;
      *p = b->next;
      return (void*)(b + 1);
    }
    p = &(*p)->next;
  }
  // выделяем новый блок
  block* b = mem_curr;
  b->size = size;
  mem_curr += sizeof(block) + size;
  return (void*)(b + 1);
}

void my_free(void* ptr) {
  if (!ptr)
    return;
  block* b = ((block*)ptr) - 1;
  b->next = free_list;
  free_list = b;
}

void init_allocator() {
  int fd = open("mem.bin", O_RDWR | O_CREAT, 0666);
  ftruncate(fd, MEM_SIZE);
  mem = mmap(NULL, MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  mem_curr = mem;
  close(fd);
}

int main() {
  init_allocator();

  char* buf1 = my_malloc(100);
  strcpy(buf1, "hello\n");
  write(1, buf1, 6);

  char* buf2 = my_malloc(50);
  strcpy(buf2, "bitch\n");
  write(1, buf2, 6);

  my_free(buf1);

  char* buf3 = my_malloc(80);
  strcpy(buf3, "reuse!!\n");
  write(1, buf3, 8);

  return 0;
}
