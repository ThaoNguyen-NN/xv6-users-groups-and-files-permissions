#include "kernel/types.h"
#include "user/user.h"

static int failed;

static void
check(char *what, int ok)
{
  printf("%s: %s\n", ok ? "PASS" : "FAIL", what);
  if (!ok)
    failed = 1;
}

int
main(void)
{
  int st;

  check("start as root (uid=0 gid=0)", getuid() == 0 && getgid() == 0);
  check("setuid(-1) rejected", setuid(-1) == -1 && getuid() == 0);
  check("setgid(-1) rejected", setgid(-1) == -1 && getgid() == 0);

  if (fork() == 0) {
    // root drops privileges: gid first, then uid
    check("root setgid(100) ok", setgid(100) == 0);
    check("root setuid(1000) ok", setuid(1000) == 0);
    check("uid/gid changed", getuid() == 1000 && getgid() == 100);

    // no longer root: cannot change back
    check("non-root setuid(0) denied", setuid(0) == -1);
    check("non-root setgid(0) denied", setgid(0) == -1);
    check("uid/gid unchanged after denial",
          getuid() == 1000 && getgid() == 100);

    // fork inherits
    if (fork() == 0)
      exit(!(getuid() == 1000 && getgid() == 100));
    wait(&st);
    check("fork inherits uid/gid", st == 0);

    // exec keeps uid/gid: whoami must print uid=1000 gid=100
    if (fork() == 0) {
      char *av[] = {"whoami", 0};
      exec("whoami", av);
      exit(1);
    }
    wait((int *)0);

    exit(failed);
  }
  wait(&st);
  if (st != 0)
    failed = 1;
  check("parent still root", getuid() == 0 && getgid() == 0);
  printf("uidtest: %s\n", failed ? "FAILED" : "ALL PASSED");
  exit(failed);
}
