#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "kernel/stat.h"
#include "user/user.h"

#define FILE_PATH "/ugp35_file"
#define LINK_PATH "/ugp35_link"
#define DIR_PATH  "/ugp35_dir"

#define FILE_MODE 0640
#define DIR_MODE  0750

#define FILE_UID 1000
#define FILE_GID 100

#define DIR_UID 1002
#define DIR_GID 200

static int failed;

static void
check(char *what, int ok)
{
  printf("%s: %s\n", ok ? "PASS" : "FAIL", what);
  if (!ok)
    failed = 1;
}

static void
check_file(char *path, uint size, int nlink, char *description)
{
  struct stat st;
  int ok = stat(path, &st) == 0;

  if (ok) {
    ok = st.type == T_FILE &&
         st.mode == FILE_MODE &&
         st.uid == FILE_UID &&
         st.gid == FILE_GID &&
         st.size == size &&
         st.nlink == nlink;
  }

  check(description, ok);
}

static void
check_directory(void)
{
  struct stat st;
  int ok = stat(DIR_PATH, &st) == 0;

  if (ok) {
    ok = st.type == T_DIR &&
         st.mode == DIR_MODE &&
         st.uid == DIR_UID &&
         st.gid == DIR_GID;
  }

  check("directory metadata is correct", ok);
}

static void
prepare(void)
{
  int fd;

  check("prepare runs as root", getuid() == 0);
  if (getuid() != 0)
    return;

  // Create a regular file.
  fd = open(FILE_PATH, O_CREATE | O_RDWR);
  check("create persistence test file", fd >= 0);

  if (fd >= 0) {
    check("write persistence test file",
          write(fd, "UGP35", 5) == 5);
    check("close persistence test file", close(fd) == 0);
  }

  // Create a directory.
  check("create persistence test directory",
        mkdir(DIR_PATH) == 0);

  // Set file metadata.
  check("chmod file to 0640",
        chmod(FILE_PATH, FILE_MODE) == 0);
  check("chown file to 1000:100",
        chown(FILE_PATH, FILE_UID, FILE_GID) == 0);

  // Set directory metadata.
  check("chmod directory to 0750",
        chmod(DIR_PATH, DIR_MODE) == 0);
  check("chown directory to 1002:200",
        chown(DIR_PATH, DIR_UID, DIR_GID) == 0);

  check_file(FILE_PATH, 5, 1,
             "file metadata before hard link");

  // Create a hard link.
  check("create hard link",
        link(FILE_PATH, LINK_PATH) == 0);

  check_file(FILE_PATH, 5, 2,
             "original file metadata after link");
  check_file(LINK_PATH, 5, 2,
             "hard link metadata");

  // Remove the original name.
  check("unlink original file",
        unlink(FILE_PATH) == 0);

  check("original pathname no longer exists",
        stat(FILE_PATH, &(struct stat){0}) < 0);

  check_file(LINK_PATH, 5, 1,
             "metadata preserved after unlink");

  // Truncate through the remaining hard link.
  fd = open(LINK_PATH, O_WRONLY | O_TRUNC);
  check("open file with O_TRUNC", fd >= 0);

  if (fd >= 0)
    check("close truncated file", close(fd) == 0);

  check_file(LINK_PATH, 0, 1,
             "metadata preserved after truncate");

  check_directory();

  // Wait for outstanding filesystem transactions.
  check("sync completes", sync() == 0);

  printf("persisttest: PREPARE %s\n",
         failed ? "FAILED" : "PASSED");
}

static void
verify(void)
{
  check("verify runs as root", getuid() == 0);
  if (getuid() != 0) {
    printf("persisttest: VERIFY FAILED\n");
    return;
  }

  check("original pathname remains absent",
        stat(FILE_PATH, &(struct stat){0}) < 0);

  check_file(LINK_PATH, 0, 1,
             "file metadata persisted after reboot");

  check_directory();

  printf("persisttest: VERIFY %s\n",
         failed ? "FAILED" : "PASSED");
}

int
main(int argc, char *argv[])
{
  if (argc != 2) {
    printf("usage: persisttest prepare|verify\n");
    exit(1);
  }

  if (strcmp(argv[1], "prepare") == 0) {
    prepare();
  } else if (strcmp(argv[1], "verify") == 0) {
    verify();
  } else {
    printf("usage: persisttest prepare|verify\n");
    exit(1);
  }

  exit(failed);
}
