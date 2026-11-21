#include "kernel/types.h"
#include "user/user.h"
#include "pwd.h"

// Read a password or username from console.
// Return 0 on success, -1 on EOF/error, -2 if too long.
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
main(int argc, char *argv[])
{
  char password[PWD_PASS_MAX];
  char *target;
  char *sh_argv[] = {"/bin/sh", 0};
  struct passwd *account;
  int ruid;
  int r;
  // Accept: su or su <username>.
  if (argc > 2) {
    printf("Usage: su [user]\n");
    exit(1);
  }

  // Default target is root.
  target = (argc == 2) ? argv[1] : "root";

  // Look up target account.
  account = getpwnam(target);

  if (account == 0) {
    printf("su: unknown user %s\n", target);
    exit(1);
  }

  // Check the real identity of the caller.
  ruid = getruid();

  // Only a real root caller can skip authentication.
  if (ruid != 0) {
    printf("Password: ");

    r = read_line(password, sizeof(password));

    if (r != 0 || pwd_authenticate(account, password) < 0) {
      printf("su: Authentication failed\n");
      exit(1);
    }
  }

  // Switch to the target account.
  if (pwd_set_identity(account) < 0) {
    printf("su: Cannot switch identity\n");
    exit(1);
  }

  // Start a shell under the target identity.
  exec("/bin/sh", sh_argv);

  printf("su: exec shell failed\n");
  exit(1);
}
