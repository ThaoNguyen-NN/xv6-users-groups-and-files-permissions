
#include "kernel/types.h"
#include "user/user.h"
#include "user/pwd.h"

// Đọc số thập phân không âm; trả -1 nếu rỗng hoặc sai ký tự.
static int
parse_num(char *s)
{
  int n = 0;

  if (*s == 0)
    return -1;
  for (; *s; s++) {
    if (*s < '0' || *s > '9')
      return -1;
    n = n * 10 + (*s - '0');
    if (n > 1000000)
      return -1;
  }
  return n;
}

int
main(int argc, char *argv[])
{
  char *colon;
  struct passwd *pw;
  int uid, gid, i, status = 0;

  if (argc < 3) {
    fprintf(2, "usage: chown <user | uid:gid> file...\n");
    exit(1);
  }

  if ((colon = strchr(argv[1], ':')) != 0) {
    // dạng uid:gid (số)
    *colon = 0;
    uid = parse_num(argv[1]);
    gid = parse_num(colon + 1);
    if (uid < 0 || gid < 0) {
      fprintf(2, "chown: invalid uid:gid\n");
      exit(1);
    }
  } else {
    // dạng tên user: lấy uid và gid chính trong /etc/passwd
    if ((pw = getpwnam(argv[1])) == 0) {
      fprintf(2, "chown: unknown user '%s'\n", argv[1]);
      exit(1);
    }
    uid = pw->pw_uid;
    gid = pw->pw_gid;
  }

  for (i = 2; i < argc; i++) {
    if (chown(argv[i], uid, gid) < 0) {
      fprintf(2, "chown: cannot change %s\n", argv[i]);
      status = 1;
    }
  }
  exit(status);
}
