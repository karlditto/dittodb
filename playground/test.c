#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

typedef struct {
  int id;
  char name[100];
  char email[100];
} Data;

int main(void) {
  Data d = {.id = 6969, .name = "karl", .email = "karl@example.com"};
  printf("size of strict Data: %zu\n", sizeof(Data));

  // store data
  int fd = open("test.db", O_CREAT | O_RDWR | S_IRUSR | S_IWUSR, 0755);
  if (fd == -1) {
    fd = open("test.db", O_RDWR | S_IRUSR | S_IWUSR | O_APPEND, 0755);
    errno = 0;
  }
  perror("current error state");
  printf("opened fd: %d\n", fd);
  ssize_t written = write(fd, &d, sizeof(Data));
  perror("current error state");
  printf("bytes written: %ld\n", written);
  close(fd);

  // read data
  fd = open("test.db", O_CREAT | O_RDWR | S_IRUSR | S_IWUSR, 0755);
  if (fd == -1) {
    fd = open("test.db", O_RDWR | S_IRUSR | S_IWUSR | O_APPEND, 0755);
    errno = 0;
  }
  perror("current error state");
  printf("opened fd: %d\n", fd);
  Data res;
  ssize_t readed = read(fd, &res, sizeof(Data));
  perror("current error state");
  printf("bytes read: %ld\n", written);
  printf("contents: %s\n", res.email);
  close(fd);
}
