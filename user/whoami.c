#include "kernel/types.h"
#include "user/user.h"

#include "kernel/types.h"
#include "user/user.h"
#include "user/pwd.h"

int
main(void)
{
  int uid = getuid();
  struct passwd *pw = getpwuid(uid);

  if (pw == 0) {
    printf("whoami: unknown uid %d\n", uid);
    exit(1);
  }

  printf("%s\n", pw->pw_name);
  exit(0);
}
