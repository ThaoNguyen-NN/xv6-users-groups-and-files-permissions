
#include <stdio.h>
#include "kernel/types.h"
#include "kernel/perm.h"

// Same API as kernel/defs.h
int check_permission(uint mode,
                     uint file_uid,
                     uint file_gid,
                     uint proc_uid,
                     uint proc_gid,
                     int requested);

static int passed = 0;
static int failed = 0;

static void
expect(const char *name,
       uint mode, uint file_uid, uint file_gid,
       uint proc_uid, uint proc_gid,
       int requested, int expected)
{
  int actual = check_permission(
      mode, file_uid, file_gid,
      proc_uid, proc_gid, requested);

  if(actual == expected){
    printf("PASS: %s\n", name);
    passed++;
  } else {
    printf("FAIL: %s (expected %d, got %d)\n",
           name, expected, actual);
    failed++;
  }
}

int
main(void)
{
  // Alice: UID 1000, GID 100
  // Carol: UID 1001, GID 100
  // Bob:   UID 1002, GID 200
  // Root:  UID 0, GID 0

  // OWNER
  expect("owner read",
         0600,1000,100,1000,100,PERM_READ,1);
  expect("owner write",
         0600,1000,100,1000,100,PERM_WRITE,1);
  expect("owner exec denied",
         0600,1000,100,1000,100,PERM_EXEC,0);

  // GROUP
  expect("group read",
         0640,1000,100,1001,100,PERM_READ,1);
  expect("group write denied",
         0640,1000,100,1001,100,PERM_WRITE,0);

  // OTHERS
  expect("others read",
         0644,1000,100,1002,200,PERM_READ,1);
  expect("others write denied",
         0644,1000,100,1002,200,PERM_WRITE,0);
  expect("others exec",
         0755,1000,100,1002,200,PERM_EXEC,1);

  // PRIORITY
  expect("owner cannot fallback to group",
         0040,1000,100,1000,100,PERM_READ,0);
  expect("group cannot fallback to others",
         0004,1000,100,1001,100,PERM_READ,0);

  // ROOT
  expect("root read mode 0000",
         0000,1000,100,0,0,PERM_READ,1);
  expect("root write mode 0000",
         0000,1000,100,0,0,PERM_WRITE,1);
  expect("root exec denied 0644",
         0644,1000,100,0,0,PERM_EXEC,0);
  expect("root exec allowed 0755",
         0755,1000,100,0,0,PERM_EXEC,1);
  expect("root RWX requires exec bit",
         0644,1000,100,0,0,7,0);

  // MULTIPLE PERMISSIONS
  expect("owner read and write",
         0600,1000,100,1000,100,6,1);
  expect("others read and write denied",
         0644,1000,100,1002,200,6,0);

  // INVALID REQUESTS
  expect("empty request denied",
         0777,1000,100,1000,100,0,0);
  expect("unknown permission bit denied",
         0777,1000,100,1000,100,8,0);

  // MODE MASK
  expect("ignore higher mode bits",
         0100644,1000,100,1002,200,4,1);

  printf("\npermtest: %d passed, %d failed\n",
         passed, failed);

  if(failed == 0)
    printf("permtest: ALL PASSED\n");
  else
    printf("permtest: FAILED\n");

  return failed != 0;
}
