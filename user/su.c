#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int uid;

  (void)argc;
  (void)argv;

  uid = getuid();

  printf("su: current process UID = %d\n", uid);

  if (uid == 0) {
    printf("su: SETUID PASS\n");
    exit(0);
  }

  printf("su: SETUID FAIL\n");
  exit(1);
}
