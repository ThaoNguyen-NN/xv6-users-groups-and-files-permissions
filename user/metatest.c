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
  int fd, pid, status, ok;
  struct stat st;

  check("starts with root credentials", getuid() == 0 && getgid() == 0);
  ok = stat("/README", &st) == 0;
  check("stat /README", ok);
  if (ok) {
    check("/README is a file", st.type == T_FILE);
    check("/README mode is 0644", st.mode == 0644);
    check("/README uid is 0", st.uid == 0);
    check("/README gid is 0", st.gid == 0);
  }
  ok = stat("/init", &st) == 0;
  check("stat /init", ok);
  if (ok) {
    check("/init mode is 0755", st.mode == 0755);
    check("/init uid is 0", st.uid == 0);
    check("/init gid is 0", st.gid == 0);
  }
  ok = stat("/console", &st) == 0;
  check("stat /console", ok);
  if (ok) {
    check("/console is a device", st.type == T_DEVICE);
    check("/console mode is 0666", st.mode == 0666);
    check("/console uid is 0", st.uid == 0);
    check("/console gid is 0", st.gid == 0);
  }
  ok = stat("/etc", &st) == 0;
  check("stat /etc", ok);
  if (ok) {
    check("/etc is a directory", st.type == T_DIR);
    check("/etc mode is 0755", st.mode == 0755);
    check("/etc uid is 0", st.uid == 0);
    check("/etc gid is 0", st.gid == 0);
  }

  ok = stat("/etc/passwd", &st) == 0;
  check("stat /etc/passwd", ok);
  if (ok) {
    check("/etc/passwd is a file", st.type == T_FILE);
    check("/etc/passwd mode is 0644", st.mode == 0644);
    check("/etc/passwd uid is 0", st.uid == 0);
    check("/etc/passwd gid is 0", st.gid == 0);
  }
  // Test file creation by root.
  unlink("metatest.root");

  fd = open("metatest.root", O_CREATE | O_RDWR);
  check("root creates file", fd >= 0);

  if (fd >= 0) {
    check("root writes file", write(fd, "x", 1) == 1);
    check("root closes file", close(fd) == 0);
    // Reopen the file and read metadata through its file descriptor.
    fd = open("metatest.root", O_RDONLY);
    check("root reopens file", fd >= 0);

    if (fd >= 0) {
    ok = fstat(fd, &st) == 0;
    check("fstat after reopening file", ok);

    if (ok) {
      check("reopened file is regular file", st.type == T_FILE);
      check("reopened file mode is 0644", st.mode == 0644);
      check("reopened file uid is 0", st.uid == 0);
      check("reopened file gid is 0", st.gid == 0);
    }
      check("root closes reopened file", close(fd) == 0);
  }
}
  unlink("metatest.root");

  // Test directory creation by root.
  unlink("metatest.dir");

  ok = mkdir("metatest.dir") == 0;
  check("root creates directory", ok);

  if (ok) {
    ok = stat("metatest.dir", &st) == 0;
    check("stat root-created directory", ok);

    if (ok) {
      check("root-created directory is directory", st.type == T_DIR);
      check("root-created directory mode is 0755", st.mode == 0755);
      check("root-created directory uid is 0", st.uid == 0);
      check("root-created directory gid is 0", st.gid == 0);
    }
  }

  unlink("metatest.dir");
  unlink("metatest.tmp");

  pid = fork();
  check("fork non-root test process", pid >= 0);

  if (pid == 0) {
    check("setgid(100)", setgid(100) == 0);
    check("setuid(1000)", setuid(1000) == 0);
    check("process uid/gid are 1000/100", getuid() == 1000 && getgid() == 100);

    fd = open("metatest.tmp", O_CREATE | O_RDWR);
check("non-root cannot create file in /", fd < 0);


    if (fd >= 0)
      close(fd);

    exit(failed);
  }

  if (pid > 0) {
    ok = wait(&status) >= 0;
    check("wait for child process", ok);

    if (ok)
      check("child permission test passed", status == 0);

    check("denied file does not exist",
          stat("metatest.tmp", &st) < 0);
  }
  unlink("metatest.tmp");

  printf("metatest: %s\n", failed ? "FAILED" : "ALL PASSED");
  exit(failed);
}
