#include "kernel/types.h"
#include "user/user.h"
#include "user/pwd.h"

int
main(void)
{
  int uid = getuid();
  int gid = getgid();
  struct passwd *pw = getpwuid(uid);

  if (pw)
    printf("%s (uid=%d gid=%d)\n", pw->pw_name, uid, gid); // in ten
  else
    printf("uid=%d gid=%d\n", uid, gid); // khong co ten chi in so
  exit(0);
}
