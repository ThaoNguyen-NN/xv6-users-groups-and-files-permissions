#include "kernel/types.h"
#include "user/user.h"

// Đọc chuỗi octal; trả -1 nếu rỗng, sai ký tự hoặc quá 07777.
static int
parse_octal(char *s)
{
  int n = 0;

  if (*s == 0)
    return -1;
  for (; *s; s++) {
    if (*s < '0' || *s > '7')
      return -1;
    n = n * 8 + (*s - '0');
    if (n > 07777)
      return -1;
  }
  return n;
}

int
main(int argc, char *argv[])
{
  int mode, i, status = 0;

  if (argc < 3) {
    fprintf(2, "usage: chmod <octal mode> file...\n");
    exit(1);
  }
  if ((mode = parse_octal(argv[1])) < 0) {
    fprintf(2, "chmod: invalid mode '%s'\n", argv[1]);
    exit(1);
  }
  for (i = 2; i < argc; i++) {
    if (chmod(argv[i], mode) < 0) {
      fprintf(2, "chmod: cannot change %s\n", argv[i]);
      status = 1;
    }
  }
  exit(status);
}
