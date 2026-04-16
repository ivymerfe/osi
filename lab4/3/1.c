#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define MEM_SIZE (1000 * 4096)

void* mem_start;
void* mem_curr;

typedef struct block {
  size_t size;
  struct block* next;
} block;

block* free_blocks = NULL;

void* my_malloc(size_t size) {
  block* prev = NULL;
  block* curr = free_blocks;

  while (curr) {
    if (curr->size >= size) {
      if (curr->size >= size + sizeof(block) + 1) {
        block* next_part = (block*)((char*)curr + sizeof(block) + size);
        next_part->size = curr->size - size - sizeof(block);
        next_part->next = curr->next;
        curr->size = size;
        if (prev)
          prev->next = next_part;
        else
          free_blocks = next_part;
      } else {
        if (prev)
          prev->next = curr->next;
        else
          free_blocks = curr->next;
      }
      return (void*)(curr + 1);
    }
    prev = curr;
    curr = curr->next;
  }
  block* b = mem_curr;
  b->size = size;
  mem_curr = (char*)mem_curr + sizeof(block) + size;
  return (void*)(b + 1);
}

void my_free(void* ptr) {
  if (!ptr)
    return;
  block* b = (block*)ptr - 1;
  b->next = free_blocks;
  free_blocks = b;
}

int main() {
  int fd = open("mem.bin", O_RDWR | O_CREAT, 0666);
  ftruncate(fd, MEM_SIZE);
  mem_start = mmap(NULL, MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  mem_curr = mem_start;
  close(fd);

  char* buf1 = my_malloc(100);
  strcpy(buf1, "hello\n");
  write(1, buf1, 6);

  getchar();

  char* buf2 = my_malloc(50);
  strcpy(buf2, "bitch\n");
  write(1, buf2, 6);

  my_free(buf1);
  getchar();

  char* buf3 = my_malloc(40);
  strcpy(buf3, "sss!\n");
  write(1, buf3, 7);
  my_free(buf2);
  my_free(buf3);

  return 0;
}
