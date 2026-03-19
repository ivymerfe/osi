#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define PM_PRESENT (1ULL << 63)
#define PM_SWAPPED (1ULL << 62)
#define PM_FILE_PAGE (1ULL << 61)
#define PM_SOFT_DIRTY (1ULL << 55)
#define PM_PFN_MASK ((1ULL << 55) - 1)

typedef struct {
  uint64_t pfn;
  unsigned int soft_dirty;
  unsigned int file_page;
  unsigned int swapped;
  unsigned int present;
  unsigned int reserved;
} pagemap_entry_t;

static pagemap_entry_t decode_pagemap_entry(uint64_t raw) {
  pagemap_entry_t entry;
  entry.pfn = raw & PM_PFN_MASK;
  entry.soft_dirty = (raw & PM_SOFT_DIRTY) ? 1U : 0U;
  entry.file_page = (raw & PM_FILE_PAGE) ? 1U : 0U;
  entry.swapped = (raw & PM_SWAPPED) ? 1U : 0U;
  entry.present = (raw & PM_PRESENT) ? 1U : 0U;
  entry.reserved = (raw >> 56) & 0b11111U;
  return entry;
}

static int read_pagemap_raw(int pm_fd,
                            uint64_t vaddr,
                            uint64_t page_size,
                            uint64_t* raw_out) {
  off_t offset = (off_t)((vaddr / page_size) * sizeof(uint64_t));
  ssize_t n = pread(pm_fd, raw_out, sizeof(uint64_t), offset);
  return (n == (ssize_t)sizeof(uint64_t)) ? 0 : -1;
}

void print_pagemap(pid_t pid) {
  char maps_path[256];
  char pagemap_path[256];
  snprintf(maps_path, sizeof(maps_path), "/proc/%d/maps", pid);
  snprintf(pagemap_path, sizeof(pagemap_path), "/proc/%d/pagemap", pid);

  FILE* maps = fopen(maps_path, "r");
  if (!maps) {
    perror("fopen maps");
    return;
  }
  int pm_fd = open(pagemap_path, O_RDONLY);
  if (pm_fd < 0) {
    perror("open pagemap");
    fclose(maps);
    return;
  }
  long ps = sysconf(_SC_PAGESIZE);
  if (ps <= 0) {
    fprintf(stderr, "failed to get page size\n");
    close(pm_fd);
    fclose(maps);
    return;
  }
  uint64_t page_size = (uint64_t)ps;

  printf("\n%-18s %-18s %-6s %-6s %-6s %-6s %-6s\n", "VAddr", "PFN", "Dirty",
         "File", "Swap", "Pres", "Rsrv");

  char line[1024];
  unsigned long printed = 0;
  const unsigned long max_print = 2000;

  while (fgets(line, sizeof(line), maps) != NULL) {
    unsigned long start = 0, end = 0;
    char perms[8] = {0};
    char pathname[256] = {0};

    if (sscanf(line, "%lx-%lx %7s %*s %*s %*s %255s", &start, &end, perms,
               pathname) < 3) {
      continue;
    }
    if (end <= start) {
      continue;
    }
    for (uint64_t addr = (uint64_t)start; addr < (uint64_t)end;
         addr += page_size) {
      uint64_t raw = 0;
      if (read_pagemap_raw(pm_fd, addr, page_size, &raw) != 0) {
        continue;
      }
      pagemap_entry_t e = decode_pagemap_entry(raw);
      if (!e.present && !e.swapped) {
        continue;
      }
      printf("0x%016" PRIx64 " 0x%016" PRIx64 " %-6u %-6u %-6u %-6u %-6u %s\n",
             addr, e.pfn, e.soft_dirty, e.file_page, e.swapped, e.present,
             e.reserved, pathname);
      printed++;
      if (printed >= max_print) {
        printf("limited to %lu entries\n", max_print);
        close(pm_fd);
        fclose(maps);
        return;
      }
    }
  }
  printf("%lu entries\n", printed);

  close(pm_fd);
  fclose(maps);
}

int main(void) {
  pid_t my_pid = getpid();
  printf("my pid: %d\n", my_pid);

  while (1) {
    printf("\npid: ");

    int input_pid = 0;
    if (scanf("%d", &input_pid) != 1) {
      int ch;
      while ((ch = getchar()) != '\n' && ch != EOF) {
      }
      if (ch == EOF) {
        break;
      }
      continue;
    }
    if (input_pid <= 0) {
      break;
    }
    print_pagemap((pid_t)input_pid);
  }
  return 0;
}
