
#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

#define TEST_FILE "/ugp23_file"
#define TEST_DIR  "/ugp23_dir"

static int
check(int condition, char *name)
{
  if(condition) {
    printf("PASS: %s\n", name);
    return 0;
  }
  printf("FAIL: %s\n", name);
  return 1;
}

int
main(void)
{
  int fd, pid, status;
  int failures = 0;
  char buf[6] = {0};
  struct stat st;

  printf("=== UGP-23 PERMISSION TEST ===\n");

  // Avoid overwriting previous test files.
  if(stat(TEST_FILE, &st) == 0 ||
     stat(TEST_DIR, &st) == 0) {
    printf("ERROR: test files already exist\n");
    exit(1);
  }

  // Root prepares the test file.
  fd = open(TEST_FILE, O_CREATE | O_RDWR);
  if(fd < 0) {
    printf("ERROR: cannot create test file\n");
    exit(1);
  }
  if(write(fd, "hello", 5) != 5) {
    close(fd);
    unlink(TEST_FILE);
    exit(1);
  }
  close(fd);

  if(mkdir(TEST_DIR) < 0) {
    unlink(TEST_FILE);
    exit(1);
  }

  // Verify metadata before testing permissions.
  if(stat(TEST_FILE, &st) < 0 ||
     st.uid != 0 ||
     (st.mode & 0777) != 0644) {
    printf("ERROR: expected root-owned 0644 file\n");
    goto cleanup;
  }

  if(stat(TEST_DIR, &st) < 0 ||
     st.uid != 0 ||
     (st.mode & 0777) != 0755) {
    printf("ERROR: expected root-owned 0755 directory\n");
    goto cleanup;
  }

  if(stat("/", &st) < 0 ||
     st.uid != 0 ||
     (st.mode & 0777) != 0755) {
    printf("ERROR: expected root-owned 0755 root dir\n");
    goto cleanup;
  }

  pid = fork();
  if(pid < 0) {
    printf("ERROR: fork failed\n");
    goto cleanup;
  }

  if(pid == 0) {
    // Child becomes an ordinary user.
    if(setgid(100) < 0 || setuid(1000) < 0) {
      printf("FAIL: switch identity\n");
      exit(1);
    }

    failures += check(
      getuid() == 1000 && getgid() == 100,
      "switch to ordinary user"
    );

    // 1. Read must be allowed.
    fd = open(TEST_FILE, O_RDONLY);
    int read_ok = 0;
    if(fd >= 0) {
      if(read(fd, buf, 5) == 5 &&
         strcmp(buf, "hello") == 0)
        read_ok = 1;
      close(fd);
    }
    failures += check(read_ok, "open READ allowed");

    // 2. Write must be denied.
    fd = open(TEST_FILE, O_WRONLY);
    failures += check(fd < 0, "open WRITE denied");
    if(fd >= 0)
      close(fd);

    // 3. Read + Write must be denied.
    fd = open(TEST_FILE, O_RDWR);
    failures += check(fd < 0, "open RDWR denied");
    if(fd >= 0)
      close(fd);

    // 4. Truncate requires WRITE.
    fd = open(TEST_FILE, O_RDONLY | O_TRUNC);
    failures += check(fd < 0, "open TRUNC denied");
    if(fd >= 0)
      close(fd);

    // 5. Root directory is not writable by user.
    failures += check(
      unlink(TEST_FILE) < 0,
      "unlink denied"
    );

    // 6. Searchable directory permits chdir.
    failures += check(
      chdir(TEST_DIR) == 0,
      "chdir allowed"
    );

    printf("=== RESULT: %d FAILED ===\n", failures);
    exit(failures == 0 ? 0 : 1);
  }

  if(wait(&status) < 0) {
    printf("ERROR: wait failed\n");
    status = 1;
  }

  // Root cleans up test files.
  unlink(TEST_FILE);
  unlink(TEST_DIR);

  if(status == 0)
    printf("UGP-23: ALL TESTS PASSED\n");
  else
    printf("UGP-23: TEST FAILED\n");

  exit(status);

cleanup:
  unlink(TEST_FILE);
  unlink(TEST_DIR);
  exit(1);
}
