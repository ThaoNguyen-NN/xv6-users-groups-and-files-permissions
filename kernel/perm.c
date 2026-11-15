
#include "types.h"
#include "perm.h"

// Return 1 = ALLOW, 0 = DENY

int
check_permission(uint mode, uint file_uid, uint file_gid, uint proc_uid,
                 uint proc_gid, int requested)
{
  uint allowed;

  // Validate requested permission bits
  if (requested <= 0 || (requested & ~7) != 0)
    return 0;

  // Keep only the 9 Unix permission bits
  mode &= 0777;

  // Root exception
  if (proc_uid == 0) {
    if ((requested & PERM_EXEC) && (mode & 0111) == 0)
      return 0;

    return 1;
  }

  // Owner permission
  if (proc_uid == file_uid) {
    allowed = (mode >> 6) & 7;
  }

  //  Group permission
  else if (proc_gid == file_gid) {
    allowed = (mode >> 3) & 7;
  }

  // Others permission
  else {
    allowed = mode & 7;
  }

  // Check all requested permissions
  return (allowed & (uint)requested) == (uint)requested;
}
