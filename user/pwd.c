
#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"
#include "pwd.h"

#define PASSWD_FILE "/etc/passwd"
#define LINE_MAX 512
#define READ_SIZE 64

// Vung nho luu ket qua tra cuu
static struct passwd result;

// Tach mot truong chuoi truoc dau ':'
static int
read_field(char **p, char *dest, int max)
{
    int i = 0;

    while (**p != ':' && **p != '\0') {
        if (i >= max - 1)
            return -1;

        dest[i++] = **p;
        (*p)++;
    }

    if (i == 0 || **p != ':')
        return -1;

    dest[i] = '\0';
    (*p)++;
    return 0;
}

// Chuyen UID hoac GID tu chuoi sang so
static int
read_number(char **p, int *value, int last)
{
    int n = 0;
    int digit;

    if (**p < '0' || **p > '9')
        return -1;

    while (**p >= '0' && **p <= '9') {
        digit = **p - '0';

        if (n > (2147483647 - digit) / 10)
            return -1;

        n = n * 10 + digit;
        (*p)++;
    }

    if (last) {
        if (**p != '\0')
            return -1;
    } else {
        if (**p != ':')
            return -1;
        (*p)++;
    }

    *value = n;
    return 0;
}

// Phan tich mot dong passwd
static int
parse_line(char *line, struct passwd *user)
{
    char *p = line;

    if (*p == '\0' || *p == '#')
        return -1;

    if (read_field(&p, user->pw_name,
                   sizeof(user->pw_name)) < 0)
        return -1;

    if (read_field(&p, user->pw_passwd,
                   sizeof(user->pw_passwd)) < 0)
        return -1;

    if (read_number(&p, &user->pw_uid, 0) < 0)
        return -1;

    if (read_number(&p, &user->pw_gid, 1) < 0)
        return -1;

    return 0;
}

// Kiem tra mot dong co khop yeu cau khong
// type = 0: ten, 1: UID, 2: GID
static int
match_line(char *line, int type,
           const char *name, int id)
{
    struct passwd temp;
    int match = 0;

    if (parse_line(line, &temp) < 0)
        return 0;

    if (type == 0)
        match = (strcmp(temp.pw_name, name) == 0);
    else if (type == 1)
        match = (temp.pw_uid == id);
    else if (type == 2)
        match = (temp.pw_gid == id);

    if (match)
        result = temp;

    return match;
}

// Ham tra cuu chung: doc file theo tung khoi
static struct passwd *
lookup(int type, const char *name, int id)
{
    int fd;
    int n;
    int used = 0;
    int too_long = 0;
    int i;
    char block[READ_SIZE];
    char line[LINE_MAX];

    fd = open(PASSWD_FILE, O_RDONLY);

    if (fd < 0)
        return 0;

    while ((n = read(fd, block, sizeof(block))) > 0) {
        for (i = 0; i < n; i++) {
            char c = block[i];

            if (c == '\r')
                continue;

            if (c == '\n') {
                if (!too_long) {
                    line[used] = '\0';

                    if (match_line(line, type, name, id)) {
                        close(fd);
                        return &result;
                    }
                }

                used = 0;
                too_long = 0;
                continue;
            }

            if (!too_long) {
                if (used < LINE_MAX - 1)
                    line[used++] = c;
                else
                    too_long = 1;
            }
        }
    }

    // Xu ly dong cuoi khong co ky tu xuong dong
    if (n == 0 && !too_long && used > 0) {
        line[used] = '\0';

        if (match_line(line, type, name, id)) {
            close(fd);
            return &result;
        }
    }

    close(fd);
    return 0;
}

// Tim theo ten
struct passwd *
getpwnam(const char *name)
{
    if (name == 0)
        return 0;

    return lookup(0, name, 0);
}

// Tim theo UID
struct passwd *
getpwuid(int uid)
{
    if (uid < 0)
        return 0;

    return lookup(1, 0, uid);
}

// Tim user dau tien co GID tuong ung
struct passwd *
getpwgid(int gid)
{
    if (gid < 0)
        return 0;

    return lookup(2, 0, gid);
}
