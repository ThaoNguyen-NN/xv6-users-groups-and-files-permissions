

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"
#include "pwd.h"

static int failed = 0;
static int passed = 0;

static void
check(int condition, char *message)
{
  if (condition) {
    printf("PASS: %s\n", message);
    passed++;
  } else {
    printf("FAIL: %s\n", message);
    failed++;
  }
}

static int
create_fixture(int *created_dir)
{
  struct stat st;
  int fd;

  *created_dir = 0;

  if (stat("/etc/passwd", &st) == 0) {
    printf("Fixture: using existing /etc/passwd\n");
    return 0;
  }

  if (mkdir("/etc") == 0)
    *created_dir = 1;

  fd = open("/etc/passwd", O_CREATE | O_WRONLY);
  if (fd < 0)
    return -1;

  char *data =
    "root:root123:0:0\n"
    "alice:alice123:1000:100\n"
    "carol:carol123:1001:100\n"
    "bob:bob123:1002:200\n"
    "invalid:pwd:abc:200\n"
    "longuser:abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
    ":1100:300\n";

  int len = strlen(data);
  int written = write(fd, data, len);
  close(fd);

  if (written != len) {
    unlink("/etc/passwd");
    if (*created_dir)
      unlink("/etc");
    return -1;
  }

  printf("Fixture: temporary /etc/passwd created\n");
  return 1;
}

int
main(int argc, char *argv[])
{
  struct passwd *p;
  struct stat st;
  int created_file = 0;
  int created_dir = 0;
  int use_fixture = 0;

  if (argc > 1 && strcmp(argv[1], "--fixture") == 0)
    use_fixture = 1;

  printf("\n=== UGP-18 PWDTEST ===\n");

  if (use_fixture) {
    int r = create_fixture(&created_dir);
    if (r < 0) {
      printf("FAIL: cannot prepare fixture\n");
      printf("pwdtest: FAILED\n");
      exit(1);
    }
    created_file = (r == 1);
  }

  if (stat("/etc/passwd", &st) < 0) {
    printf("FAIL: /etc/passwd does not exist\n");
    printf("Use pwdtest --fixture before UGP-19 integration\n");
    printf("pwdtest: FAILED\n");
    exit(1);
  }

  // T01: root
  p = getpwnam("root");
  check(p != 0 && p->pw_uid == 0 && p->pw_gid == 0, "find root");

  // T02: alice
  p = getpwnam("alice");
  check(p != 0 && p->pw_uid == 1000 && p->pw_gid == 100, "find alice");

  // T03: carol
  p = getpwnam("carol");
  check(p != 0 && p->pw_uid == 1001 && p->pw_gid == 100, "find carol");

  // T04: bob
  p = getpwnam("bob");
  check(p != 0 && p->pw_uid == 1002 && p->pw_gid == 200, "find bob");

  // T05: UID 1000
  p = getpwuid(1000);
  check(p != 0 && strcmp(p->pw_name, "alice") == 0,
        "getpwuid(1000) returns alice");

  // T06: ten khong ton tai
  p = getpwnam("unknown");
  check(p == 0, "unknown user returns NULL");

  // T07: UID khong ton tai
  p = getpwuid(9999);
  check(p == 0, "unknown UID returns NULL");

  // T08: tim theo GID
  p = getpwgid(100);
  check(p != 0 && p->pw_gid == 100, "find user by GID 100");

  // T09, T10: chi test duoc khi co du lieu fixture
  if (created_file) {
    p = getpwnam("longuser");
    check(p != 0 && p->pw_uid == 1100 && strlen(p->pw_passwd) == 124,
          "long line across read blocks");

    p = getpwnam("invalid");
    check(p == 0, "malformed passwd line ignored");
  } else {
    printf("SKIP: fixture-only long/malformed tests\n");
  }

  // Don dep chi khi chinh test tao du lieu
  if (created_file) {
    unlink("/etc/passwd");
    if (created_dir)
      unlink("/etc");
  }

  printf("\nRESULT: %d passed, %d failed\n", passed, failed);

  if (failed == 0) {
    printf("pwdtest: ALL PASSED\n");
    exit(0);
  }

  printf("pwdtest: FAILED\n");
  exit(1);
}
