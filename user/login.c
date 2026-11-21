#include "kernel/types.h"
#include "user/user.h"
#include "pwd.h"

// Doc mot dong tu console
// Tra ve 0 neu thanh cong, -1 neu EOF/loi,
// -2 neu chuoi qua dai.
static int
read_line(char *buf, int max)
{
  char c;
  int n = 0;
  int overflow = 0;

  for (;;) {
    int r = read(0, &c, 1);

    if (r <= 0)
      return -1;

    if (c == '\r')
      continue;

    if (c == '\n') {
      buf[n] = '\0';
      return overflow ? -2 : 0;
    }

    if (n < max - 1)
      buf[n++] = c;
    else
      overflow = 1;
  }
}

int
main(void)
{
  char username[PWD_NAME_MAX];
  char password[PWD_PASS_MAX];
  char *argv[] = {"sh", 0};
  struct passwd *account;
  int r;

  for (;;) {
    printf("login: ");

    r = read_line(username, sizeof(username));
    if (r == -1)
      exit(1);

    if (r != 0) {
      printf("Login incorrect\n");
      continue;
    }

    printf("Password: ");

    r = read_line(password, sizeof(password));
    if (r == -1)
      exit(1);

    if (r != 0) {
      printf("Login incorrect\n");
      continue;
    }

    // Tra cuu tai khoan trong /etc/passwd
    account = getpwnam(username);

    // Tu choi user khong ton tai hoac sai mat khau
    if (account == 0 || pwd_authenticate(account, password) < 0) {
      printf("Login incorrect\n");
      continue;
    }

    // Doi GID va UID theo tai khoan
    if (pwd_set_identity(account) < 0) {
      printf("login: cannot switch identity\n");
      exit(1);
    }

    // Chay shell duoi quyen user vua dang nhap
    exec("/bin/sh", argv);

    printf("login: exec sh failed\n");
    exit(1);
  }
}
