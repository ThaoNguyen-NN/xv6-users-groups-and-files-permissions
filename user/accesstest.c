
#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

#define READ_TEST  0
#define WRITE_TEST 1
#define EXEC_TEST  2

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

struct identity roles[] = {{"Owner", 1000, 100},
                           {"Group", 1001, 100},
                           {"Other", 1002, 200},
                           {"Root", 0, 0}};

struct fixture files[] = {{"0600", "/perm600", 0600},
                          {"0644", "/perm644", 0644},
                          {"0755", "/perm755", 0755}};

char *operations[] = {"READ", "WRITE", "EXEC"};

// Expected results: 1 = ALLOW, 0 = DENY.
// Dimensions: [mode][role][operation].
int expected[3][4][3] = {{
                           // 0600
                           {1, 1, 0}, // Owner
                           {0, 0, 0}, // Group
                           {0, 0, 0}, // Other
                           {1, 1, 0}  // Root
                         },
                         {
                           // 0644
                           {1, 1, 0}, // Owner
                           {1, 0, 0}, // Group
                           {1, 0, 0}, // Other
                           {1, 1, 0}  // Root
                         },
                         {
                           // 0755
                           {1, 1, 1}, // Owner
                           {1, 0, 1}, // Group
                           {1, 0, 1}, // Other
                           {1, 1, 1}  // Root
                         }};

// Run one actual filesystem operation.
void
perform_operation(char *path, int op, char first_byte)
{
  int fd;
  char buf;

  if (op == READ_TEST) {
    fd = open(path, O_RDONLY);
    if (fd < 0)
      exit(1);

    int n = read(fd, &buf, 1);
    close(fd);
    exit(n == 1 ? 0 : 1);
  }

  if (op == WRITE_TEST) {
    fd = open(path, O_WRONLY);
    if (fd < 0)
      exit(1);

    // Rewrite the original first byte so the ELF
    // remains valid for subsequent EXEC tests.
    int n = write(fd, &first_byte, 1);
    close(fd);
    exit(n == 1 ? 0 : 1);
  }

  if (op == EXEC_TEST) {
    char *argv[] = {"permfixture", "EXEC_OK", 0};

    // Success replaces this process with the echo ELF.
    // An unsuccessful exec returns here.
    exec(path, argv);
    exit(1);
  }

  exit(2);
}

// Fork a fresh process for every test case.
int
run_case(int fi, int ri, int op, char first_byte)
{
  int pid, status;
  int actual;
  int want = expected[fi][ri][op];

  pid = fork();
  if (pid < 0) {
    printf("FAIL: fork error\n");
    return 1;
  }

  if (pid == 0) {
    // Parent is Root. Child switches identity.
    if (roles[ri].uid != 0 || roles[ri].gid != 0) {
      if (setgid(roles[ri].gid) < 0 || setuid(roles[ri].uid) < 0)
        exit(2);
    }

    if (getuid() != roles[ri].uid || getgid() != roles[ri].gid)
      exit(2);

    perform_operation(files[fi].path, op, first_byte);
    exit(2);
  }

  if (wait(&status) != pid) {
    printf("FAIL: wait error\n");
    return 1;
  }

  if (status == 2) {
    printf("FAIL: %s %s %s identity/setup error\n", files[fi].name,
           roles[ri].name, operations[op]);
    return 1;
  }

  actual = (status == 0);

  if (actual == want) {
    printf("PASS: %s %s %s => %s\n", files[fi].name, roles[ri].name,
           operations[op], actual ? "ALLOW" : "DENY");
    return 0;
  }

  printf("FAIL: %s %s %s expected %s got %s\n", files[fi].name, roles[ri].name,
         operations[op], want ? "ALLOW" : "DENY", actual ? "ALLOW" : "DENY");
  return 1;
}

int
main(void)
{
  int fi, ri, op;
  int total = 0;
  int failures = 0;
  char first_bytes[3];
  struct stat st;

  printf("=== UGP-26 ACCESS TEST ===\n");

  if (getuid() != 0 || getgid() != 0) {
    printf("ERROR: run accesstest as Root\n");
    exit(1);
  }

  // Verify fixture metadata and read original ELF bytes.
  for (fi = 0; fi < 3; fi++) {
    if (stat(files[fi].path, &st) < 0) {
      printf("ERROR: missing %s\n", files[fi].path);
      exit(1);
    }

    if (st.uid != 1000 || st.gid != 100 || (st.mode & 0777) != files[fi].mode) {
      printf("ERROR: incorrect metadata for %s\n", files[fi].path);
      exit(1);
    }

    int fd = open(files[fi].path, O_RDONLY);
    if (fd < 0) {
      printf("ERROR: cannot read fixture\n");
      exit(1);
    }

    int n = read(fd, &first_bytes[fi], 1);
    close(fd);

    if (n != 1 || first_bytes[fi] != 0x7f) {
      printf("ERROR: invalid ELF fixture\n");
      exit(1);
    }
  }

  printf("Fixtures verified: 0600, 0644, 0755\n");

  for (fi = 0; fi < 3; fi++) {
    printf("\n=== MODE %s ===\n", files[fi].name);

    for (ri = 0; ri < 4; ri++) {
      for (op = 0; op < 3; op++) {
        failures += run_case(fi, ri, op, first_bytes[fi]);
        total++;
      }
    }
  }

  printf("\n=== UGP-26 SUMMARY ===\n");
  printf("Total: %d\n", total);
  printf("Passed: %d\n", total - failures);
  printf("Failed: %d\n", failures);

  if (failures == 0) {
    printf("UGP-26: ALL TESTS PASSED\n");
    exit(0);
  }

  printf("UGP-26: TESTS FAILED\n");
  exit(1);
}
