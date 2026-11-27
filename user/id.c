#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  printf("uid=%d gid=%d ruid=%d\n", getuid(), getgid(), getruid());
  exit(0);
}
