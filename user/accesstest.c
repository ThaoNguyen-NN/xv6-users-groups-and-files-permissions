#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

#define MODE_COUNT 6
#define ROLE_COUNT 4
#define OP_COUNT   3

#define READ_TEST  0
#define WRITE_TEST 1
#define EXEC_TEST  2

#define SEARCH_TEST 0
#define CREATE_TEST 1
#define DELETE_TEST 2

#define DIR_PATH       "/matrixdir"
#define DIR_PROBE_PATH "/matrixdir/probe"

struct identity {
  char *name;
  int uid;
  int gid;
};

struct fixture {
  char *name;
  char *path;
  int mode;
};

struct identity roles[ROLE_COUNT] = {{"Owner", 1000, 100},
                                     {"Group", 1001, 100},
                                     {"Other", 1002, 200},
                                     {"Root", 0, 0}};

struct fixture files[MODE_COUNT] = {
  {"000", "/perm000", 0000}, {"600", "/perm600", 0600},
  {"640", "/perm640", 0640}, {"644", "/perm644", 0644},
  {"755", "/perm755", 0755}, {"4755", "/perm4755", 04755}};

char *file_operations[OP_COUNT] = {"READ", "WRITE", "EXEC"};

char *dir_operations[OP_COUNT] = {"SEARCH", "CREATE", "DELETE"};

// Expected results: 1 = ALLOW, 0 = DENY.
// Dimensions: [mode][role][operation].
int expected_file[MODE_COUNT][ROLE_COUNT][OP_COUNT] = {
  // Mode 000
  {
    {0, 0, 0}, // Owner
    {0, 0, 0}, // Group
    {0, 0, 0}, // Other
    {1, 1, 0}  // Root
  },

  // Mode 600
  {{1, 1, 0}, {0, 0, 0}, {0, 0, 0}, {1, 1, 0}},

  // Mode 640
  {{1, 1, 0}, {1, 0, 0}, {0, 0, 0}, {1, 1, 0}},

  // Mode 644
  {{1, 1, 0}, {1, 0, 0}, {1, 0, 0}, {1, 1, 0}},

  // Mode 755
  {{1, 1, 1}, {1, 0, 1}, {1, 0, 1}, {1, 1, 1}},

  // Mode 4755: same rwx access as 0755
  {{1, 1, 1}, {1, 0, 1}, {1, 0, 1}, {1, 1, 1}}};

// Expected directory operations:
// SEARCH requires execute.
// CREATE and DELETE require write + execute.
int expected_dir[MODE_COUNT][ROLE_COUNT][OP_COUNT] = {
  // Mode 000
  {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},

  // Mode 600
  {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},

  // Mode 640
  {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},

  // Mode 644
  {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},

  // Mode 755
  {
    {1, 1, 1}, // Owner
    {1, 0, 0}, // Group
    {1, 0, 0}, // Other
    {1, 1, 1}  // Root
  },

  // Mode 4755: same rwx access as 0755
  {{1, 1, 1}, {1, 0, 0}, {1, 0, 0}, {1, 1, 1}}};

// Print Expected, Actual and PASS/FAIL for every case.
// actual = -1 means the test itself encountered an error.
int
report_result(char *kind, char *mode, char *role, char *operation, int expected,
              int actual)
{
  int passed = (actual == expected);

  printf("%s mode=%s role=%s op=%s Expected=%s Actual=%s %s\n", kind, mode,
         role, operation, expected ? "ALLOW" : "DENY",
         actual < 0 ? "ERROR" : (actual ? "ALLOW" : "DENY"),
         passed ? "PASS" : "FAIL");

  return passed ? 0 : 1;
}

// Change the child process to the identity being tested.
// Exit status 2 indicates a setup error.
void
switch_identity(int ri)
{
  if (roles[ri].uid != 0 || roles[ri].gid != 0) {
    if (setgid(roles[ri].gid) < 0 || setuid(roles[ri].uid) < 0)
      exit(2);
  }

  if (getuid() != roles[ri].uid || getgid() != roles[ri].gid)
    exit(2);
}

// Perform one file operation.
void
perform_file_operation(int fi, int op, char first_byte)
{
  int fd;
  int n;
  char buf;

  if (op == READ_TEST) {
    fd = open(files[fi].path, O_RDONLY);
    if (fd < 0)
      exit(1);

    n = read(fd, &buf, 1);
    close(fd);
    exit(n == 1 ? 0 : 1);
  }

  if (op == WRITE_TEST) {
    fd = open(files[fi].path, O_WRONLY);
    if (fd < 0)
      exit(1);

    // Restore the original first byte so the ELF remains valid.
    n = write(fd, &first_byte, 1);
    close(fd);
    exit(n == 1 ? 0 : 1);
  }

  if (op == EXEC_TEST) {
    char *argv[] = {"permfixture", "EXEC_OK", 0};

    // Successful exec replaces this process with the fixture.
    exec(files[fi].path, argv);
    exit(1);
  }

  exit(2);
}

// Run one file test in a child process.
int
run_file_case(int fi, int ri, int op, char first_byte)
{
  int pid;
  int status;
  int actual;
  int want = expected_file[fi][ri][op];

  pid = fork();

  if (pid < 0) {
    return report_result("FILE", files[fi].name, roles[ri].name,
                         file_operations[op], want, -1);
  }

  if (pid == 0) {
    switch_identity(ri);
    perform_file_operation(fi, op, first_byte);
    exit(2);
  }

  if (wait(&status) != pid)
    actual = -1;
  else if (status == 2)
    actual = -1;
  else
    actual = (status == 0);

  return report_result("FILE", files[fi].name, roles[ri].name,
                       file_operations[op], want, actual);
}

// Remove a previous directory fixture, if one exists.
// The parent process runs as root.
void
cleanup_directory(void)
{
  chmod(DIR_PATH, 0700);
  unlink(DIR_PROBE_PATH);
  unlink(DIR_PATH);
}

// Prepare a directory fixture for one operation.
// Directory owner: UID 1000, GID 100.
int
prepare_directory_case(int mode, int op)
{
  int fd;

  cleanup_directory();

  if (mkdir(DIR_PATH) < 0)
    return -1;

  // Temporarily make the directory writable/searchable by root.
  if (chmod(DIR_PATH, 0700) < 0) {
    cleanup_directory();
    return -1;
  }

  // DELETE needs an existing file inside the directory.
  if (op == DELETE_TEST) {
    fd = open(DIR_PROBE_PATH, O_CREATE | O_WRONLY);
    if (fd < 0) {
      cleanup_directory();
      return -1;
    }
    close(fd);
  }

  if (chown(DIR_PATH, 1000, 100) < 0) {
    cleanup_directory();
    return -1;
  }

  if (chmod(DIR_PATH, mode) < 0) {
    cleanup_directory();
    return -1;
  }

  return 0;
}

// Perform one directory operation.
void
perform_directory_operation(int op)
{
  int fd;

  if (op == SEARCH_TEST) {
    exit(chdir(DIR_PATH) == 0 ? 0 : 1);
  }

  if (op == CREATE_TEST) {
    fd = open(DIR_PROBE_PATH, O_CREATE | O_WRONLY);
    if (fd < 0)
      exit(1);

    close(fd);
    exit(0);
  }

  if (op == DELETE_TEST) {
    exit(unlink(DIR_PROBE_PATH) == 0 ? 0 : 1);
  }

  exit(2);
}

// Run one directory test in a child process.
int
run_directory_case(int fi, int ri, int op)
{
  int pid;
  int status;
  int actual;
  int want = expected_dir[fi][ri][op];

  if (prepare_directory_case(files[fi].mode, op) < 0) {
    cleanup_directory();
    return report_result("DIR", files[fi].name, roles[ri].name,
                         dir_operations[op], want, -1);
  }

  pid = fork();

  if (pid < 0) {
    cleanup_directory();
    return report_result("DIR", files[fi].name, roles[ri].name,
                         dir_operations[op], want, -1);
  }

  if (pid == 0) {
    switch_identity(ri);
    perform_directory_operation(op);
    exit(2);
  }

  if (wait(&status) != pid)
    actual = -1;
  else if (status == 2)
    actual = -1;
  else
    actual = (status == 0);

  // Restore root access and remove the test directory.
  cleanup_directory();

  return report_result("DIR", files[fi].name, roles[ri].name,
                       dir_operations[op], want, actual);
}

int
main(void)
{
  int fi;
  int ri;
  int op;
  int total = 0;
  int failures = 0;
  char first_bytes[MODE_COUNT];
  struct stat st;

  printf("=== UGP-38 PERMISSION MATRIX ===\n");

  if (getuid() != 0 || getgid() != 0) {
    printf("ERROR: run accesstest as Root\n");
    exit(1);
  }

  // Verify all six file fixtures and save their original ELF bytes.
  for (fi = 0; fi < MODE_COUNT; fi++) {
    if (stat(files[fi].path, &st) < 0) {
      printf("ERROR: missing fixture %s\n", files[fi].path);
      exit(1);
    }

    if (st.uid != 1000 || st.gid != 100 ||
        (st.mode & 07777) != files[fi].mode) {
      printf("ERROR: incorrect metadata for %s\n", files[fi].path);
      exit(1);
    }

    int fd = open(files[fi].path, O_RDONLY);
    if (fd < 0) {
      printf("ERROR: cannot read fixture %s\n", files[fi].path);
      exit(1);
    }

    int n = read(fd, &first_bytes[fi], 1);
    close(fd);

    if (n != 1 || first_bytes[fi] != 0x7f) {
      printf("ERROR: invalid ELF fixture %s\n", files[fi].path);
      exit(1);
    }
  }

  printf("File fixtures verified: 000, 600, 640, 644, 755, 4755\n");
  printf("File owner: UID=1000 GID=100\n");

  // File matrix: 6 modes x 4 roles x 3 operations = 72 cases.
  printf("\n=== FILE PERMISSION TESTS ===\n");

  for (fi = 0; fi < MODE_COUNT; fi++) {
    for (ri = 0; ri < ROLE_COUNT; ri++) {
      for (op = 0; op < OP_COUNT; op++) {
        failures += run_file_case(fi, ri, op, first_bytes[fi]);
        total++;
      }
    }
  }

  // Directory matrix: 6 modes x 4 roles x 3 operations = 72 cases.
  printf("\n=== DIRECTORY PERMISSION TESTS ===\n");

  for (fi = 0; fi < MODE_COUNT; fi++) {
    for (ri = 0; ri < ROLE_COUNT; ri++) {
      for (op = 0; op < OP_COUNT; op++) {
        failures += run_directory_case(fi, ri, op);
        total++;
      }
    }
  }

  printf("\n=== UGP-38 SUMMARY ===\n");
  printf("Total: %d\n", total);
  printf("Passed: %d\n", total - failures);
  printf("Failed: %d\n", failures);

  if (failures == 0) {
    printf("ALL TESTS PASSED\n");
    exit(0);
  }

  printf("UGP-38: TESTS FAILED\n");
  exit(1);
}
