#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "kernel/stat.h"
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
  int fd, pid, status;
  struct stat st;

  // Remove files left by an earlier test.
  unlink("createtest.root");
  unlink("createtest.user");

  // Root should be able to create a file in root directory.
  fd = open("createtest.root", O_CREATE | O_RDWR);
  check("root creates file in /", fd >= 0);

  if (fd >= 0)
    check("root closes file", close(fd) == 0);

  // Test a non-root process.
  pid = fork();
  check("fork non-root process", pid >= 0);

  if (pid == 0) {
    check("setgid(100)", setgid(100) == 0);
    check("setuid(1000)", setuid(1000) == 0);

    fd = open("createtest.user", O_CREATE | O_RDWR);

    check("non-root cannot create file in /", fd < 0);

    if (fd >= 0) {
      close(fd);
      unlink("createtest.user");
    }

    exit(failed);
  }

  if (pid > 0) {
    check("wait for child", wait(&status) >= 0);
    check("child permission test passed", status == 0);

    check("denied file does not exist",
          stat("createtest.user", &st) < 0);
  }

  // Clean up.
  unlink("createtest.root");
  unlink("createtest.user");

  printf("createtest: %s\n", failed ? "FAILED" : "ALL PASSED");
  exit(failed);
}
