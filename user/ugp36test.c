#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

#define TEST_DIR  "/ugp36testdir"
#define TEST_FILE "/ugp36testdir/data"
#define NEW_FILE  "/ugp36testdir/newfile"

static int failures;

static void
check(char *name, int ok)
{
  printf("%s: %s\n", ok ? "PASS" : "FAIL", name);
  if (!ok)
    failures++;
}

int
main(void)
{
  int fd, pid, status;
  struct stat st;

  printf("=== UGP-36 ROOT PERMISSION TEST ===\n");

  if (getuid() != 0) {
    printf("ERROR: run as root\n");
    exit(1);
  }

  if (mkdir(TEST_DIR) < 0) {
    printf("ERROR: cannot create test directory\n");
    exit(1);
  }

  fd = open(TEST_FILE, O_CREATE | O_WRONLY);
  if (fd < 0) {
    unlink(TEST_DIR);
    exit(1);
  }
  close(fd);

  if (chmod(TEST_DIR, 0000) < 0) {
    unlink(TEST_FILE);
    unlink(TEST_DIR);
    exit(1);
  }

  check("Root chdir directory mode 000", chdir(TEST_DIR) == 0);

  chdir("/");

  check("Root unlink inside directory mode 000", unlink(TEST_FILE) == 0);

  check("Deleted file no longer exists", stat(TEST_FILE, &st) < 0);

  fd = open(NEW_FILE, O_CREATE | O_WRONLY);

  check("Root create inside directory mode 000", fd >= 0);

  if (fd >= 0)
    close(fd);

  pid = fork();

  if (pid == 0) {
    if (setgid(100) < 0 || setuid(1000) < 0)
      exit(2);

    if (chdir(TEST_DIR) == 0)
      exit(1);

    if (unlink(NEW_FILE) == 0)
      exit(1);

    exit(0);
  }

  if (pid > 0) {
    check("Non-root chdir and unlink denied",
          wait(&status) == pid && status == 0);
  } else {
    check("Fork test process", 0);
  }

  chmod(TEST_DIR, 0755);
  unlink(TEST_FILE);
  unlink(NEW_FILE);
  unlink(TEST_DIR);

  if (failures == 0)
    printf("UGP-36: ALL TESTS PASSED\n");
  else
    printf("UGP-36: %d TESTS FAILED\n", failures);

  exit(failures != 0);
}
