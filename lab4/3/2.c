#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define MEM_SIZE (1024 * 1024 * 10)
#define BLOCK_SIZE 1024
#define NUM_BLOCKS (MEM_SIZE / BLOCK_SIZE)
#define BITMAP_WORDS (NUM_BLOCKS / 64)

void* mem_start;
uint64_t* bitmap;
void* data_start;

void set_bits(int start_bit, int count) {
  for (int i = 0; i < count; i++) {
    int idx = start_bit + i;
    bitmap[idx / 64] |= (1ULL << (idx % 64));
  }
}

void unset_bits(int start_bit, int count) {
  for (int i = 0; i < count; i++) {
    int idx = start_bit + i;
    bitmap[idx / 64] &= ~(1ULL << (idx % 64));
  }
}

void* my_malloc(size_t size) {
  int blocks_needed = (size + sizeof(uint32_t) + BLOCK_SIZE - 1) / BLOCK_SIZE;
  int cons = 0;
  int start_bit = -1;

  for (int i = 0; i < NUM_BLOCKS; i++) {
    if (!(bitmap[i / 64] & (1ULL << (i % 64)))) {
      if (cons == 0) {
        start_bit = i;
      }
      cons++;

      if (cons == blocks_needed) {
        set_bits(start_bit, blocks_needed);
        uint32_t* header =
            (uint32_t*)((char*)data_start + (start_bit * BLOCK_SIZE));
        *header = blocks_needed;
        return (void*)(header + 1);
      }
    } else {
      cons = 0;
    }
  }
  return NULL;
}

void my_free(void* ptr) {
  if (!ptr)
    return;

  uint32_t* header = (uint32_t*)ptr - 1;
  int blocks_to_free = *header;
  size_t offset = (size_t)((char*)header - (char*)data_start);
  int start_index = offset / BLOCK_SIZE;

  unset_bits(start_index, blocks_to_free);
}

void init_malloc() {
  int fd = open("mem.bin", O_RDWR | O_CREAT, 0666);
  ftruncate(fd, MEM_SIZE);
  mem_start = mmap(NULL, MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  close(fd);

  bitmap = (uint64_t*)mem_start;
  data_start = (void*)((char*)mem_start + (BITMAP_WORDS * sizeof(uint64_t)));
}

int main() {
  init_malloc();

  char* buf1 = my_malloc(100);
  strcpy(buf1, "hello\n");
  write(1, buf1, 6);

  char* buf2 = my_malloc(50);
  strcpy(buf2, "bitch\n");
  write(1, buf2, 6);

  my_free(buf1);

  char* buf3 = my_malloc(40);
  strcpy(buf3, "sss!\n");
  write(1, buf3, 7);
  my_free(buf2);
  my_free(buf3);
  return 0;
}
