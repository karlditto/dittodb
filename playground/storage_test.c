#include <assert.h>
#include <endian.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#define PAGE_SIZE 256

// on disk page structure (slotted page)
typedef struct {
  unsigned short slotno;
  unsigned short offset;
} slot;

typedef struct {
  // header
  unsigned long long pageno;
  unsigned short maxslotno;

  // data section
  char data[PAGE_SIZE - sizeof(unsigned long long) - sizeof(unsigned short)];
} dbpage;

typedef struct {
  unsigned long long id;
  char name[8];
  char email[32];
} table;

void insert(dbpage *page, table row) {
  slot *slt = malloc(sizeof(slot));
  slt->slotno = ++page->maxslotno;
  slt->offset = PAGE_SIZE - sizeof(unsigned long long) * 2 -
                page->maxslotno * sizeof(table);
  memcpy(page->data + (page->maxslotno - 1) * sizeof(slot), slt, sizeof(slot));
  memcpy(page->data + slt->offset, &row, sizeof(table));
}

int main(void) {
  dbpage page = {0};
  page.pageno = 0;
  table row1 = {
      .id = 1,
      .name = "david",
      .email = "david@example.com",
  };
  table row2 = {
      .id = 2,
      .name = "karl",
      .email = "karl@example.com",
  };
  table row3 = {
      .id = 3,
      .name = "tom",
      .email = "tom@example.com",
  };
  printf("size of page:%zu\n", sizeof(dbpage));
  printf("size of row:%zu\n", sizeof(table));
  printf("size of slot:%zu\n", sizeof(slot));
  // memcpy(page.data, &row1, sizeof(table));
  // memcpy(page.data + sizeof(table), &row2, sizeof(table));
  // memcpy(page.data + sizeof(table) * 2, &row3, sizeof(table));
  insert(&page, row1);
  insert(&page, row2);
  insert(&page, row3);

  int fd = open("storage_test.db", O_CREAT | O_RDWR | S_IRUSR | S_IWUSR, 0755);
  if (fd == -1) {
    fd = open("storage_test.db", O_RDWR | S_IRUSR | S_IWUSR | O_TRUNC, 0755);
    errno = 0;
  }
  perror("current error state");
  printf("opened fd: %d\n", fd);
  ssize_t written = write(fd, &page, sizeof(page));
  perror("current error state");
  printf("bytes written: %ld\n", written);
  close(fd);

  // dbpage page2 = {0};
  // page2.pageno = 1;
  // fd = open("storage_test.db", O_RDWR | S_IRUSR | S_IWUSR | O_APPEND, 0755);
  // errno = 0;
  // perror("current error state");
  // printf("opened fd: %d\n", fd);
  // written = write(fd, &page2, sizeof(page));
  // perror("current error state");
  // printf("bytes written: %ld\n", written);

  fd = open("storage_test.db", O_CREAT | O_RDWR | S_IRUSR | S_IWUSR, 0755);
  if (fd == -1) {
    fd = open("storage_test.db", O_RDWR, 0755);
    errno = 0;
  }

  struct stat statbuf;
  int err = fstat(fd, &statbuf);
  assert(err >= 0);
  void *ptr =
      mmap(NULL, statbuf.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

  dbpage *read_page = (dbpage *)ptr;

  slot *read_slot = (slot *)(read_page->data + sizeof(slot));
  printf("read slot #:%hu", read_slot->slotno);
  printf("read slot offset:%hu", read_slot->offset);

  munmap(ptr, statbuf.st_size);

  return 0;
}
