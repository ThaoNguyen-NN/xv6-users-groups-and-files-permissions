
#ifndef USER_PWD_H
#define USER_PWD_H

#define PWD_NAME_MAX 32
#define PWD_PASS_MAX 256

// Thong tin cua mot tai khoan
struct passwd {
  char pw_name[PWD_NAME_MAX];
  char pw_passwd[PWD_PASS_MAX];
  int pw_uid;
  int pw_gid;
};

// Tim user theo ten
struct passwd *getpwnam(const char *name);

// Tim user theo UID
struct passwd *getpwuid(int uid);

// Tim user dau tien co GID phu hop
struct passwd *getpwgid(int gid);

#endif
