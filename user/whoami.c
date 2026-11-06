#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  printf("uid=%d gid=%d\n", getuid(), getgid());
  exit(0);
}
