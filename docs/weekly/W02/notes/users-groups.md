# UGP-11: Đặc tả user/group, /etc/passwd, uid/gid trong proc, syscall mới

## 1. Mô hình chung

- Mỗi process có một cặp `uid`, `gid` (`uint`). Chưa có supplementary groups, chưa có euid/egid.
- `uid = 0` là root. Kernel chỉ biết số; tên user chỉ có trong `/etc/passwd`.
- Quyền file theo chuẩn Unix: 9 bit rwx cho owner / group / other. Cách kiểm tra thuộc UGP-6/23.
- `chown` chỉ root được dùng. `chmod` chỉ owner hoặc root.
- Bit 04000 của `mode` để dành cho setuid (xem đề xuất P2), hiện chưa có tác dụng.

## 2. /etc/passwd

### 2.1 Định dạng

Mỗi dòng một user, bốn trường cách nhau bởi `:`:

```
name:password:uid:gid
```

- `name`: tối đa 31 ký tự (bộ đệm `char name[32]`, gồm cả `\0`), không chứa `:` hoặc khoảng trắng.
- `password`: dữ liệu mật khẩu do `login` và `su` hiểu, kernel không đọc trường này. Ở giai đoạn đầu là chuỗi thuần; nếu sau này lưu dạng băm (P1) thì chỉ đổi nội dung trường, không đổi định dạng dòng.
- `uid`, `gid`: số thập phân.
- Không có `/etc/group`: nhóm chỉ là số `gid` trong dòng passwd; hai user cùng `gid` thì cùng nhóm.
- Không có trường home và shell: mọi user chạy `sh`, thư mục làm việc là `/`.

### 2.2 Bộ user demo

Đủ ba lớp cho test matrix (Owner = alice, Group = carol, Other = bob):

```
root:root:0:0
nguyen:nguyen1:1000:100
thu:thu1:1001:100
ducanh:ducanh1:1002:200
```

Mật khẩu chỉ dùng cho demo.

### 2.3 Đọc file

xv6 không có libc nên viết thư viện nhỏ `user/pwd.c`: `getpwnam(name, &entry)` và `getpwuid(uid, &entry)`, đọc bằng `open/read` rồi tách dòng thủ công. `login`, `su`, `whoami` cùng dùng. Thêm `$U/pwd.o` vào `ULIB` trong Makefile.

- Bộ đệm đọc dòng: 256 byte (dòng dài nhất khoảng 128 byte, kể cả khi dùng mật khẩu băm ở P1).
- `read()` trả dữ liệu theo khối chứ không theo dòng, nên một dòng có thể bị cắt giữa hai lần `read`. `getpwnam` phải tự ghép dòng, giữ phần dư giữa các lần đọc.

## 3. uid/gid trong struct proc

`kernel/proc.h`: thêm hai trường vào nhóm "these are private to the process, so p->lock need not be held":

```c
uint uid;
uint gid;
```

| Hàm (mã gốc) | Việc cần làm |
|---|---|
| `allocproc` | sau `p->state = USED;` đặt `p->uid = 0; p->gid = 0;`. Process đầu tiên (`init`) nhờ vậy là root. `freeproc` không cần sửa |
| `kfork` | cạnh `np->cwd = idup(p->cwd);` thêm `np->uid = p->uid; np->gid = p->gid;` (lúc đó `np->lock` đang giữ, con chưa RUNNABLE) |
| `kexec` | giữ nguyên `uid/gid` của process |
| `kexit`, `kwait`, `reparent` | không đổi |

Khóa: `uid/gid` chỉ được đọc và ghi bởi chính process (qua `myproc()`), nên không cần giữ `p->lock`. Nếu sau này phải đọc `uid` của process khác (ví dụ `kkill`) thì phải giữ `p->lock`.

Hai trường này là nguồn để gán chủ sở hữu cho inode mới và để so sánh khi kiểm tra quyền; phần cài đặt đó thuộc UGP-12 và UGP-6/23.

## 4. Syscall mới

`SYS_sync` đã chiếm số 22, nên số mới bắt đầu từ 23.

| # | Syscall | Mô tả | Quy tắc | Lỗi (trả -1) |
|---|---|---|---|---|
| 23 | `int getuid(void)` | trả `p->uid` | ai cũng gọi được | không |
| 24 | `int getgid(void)` | trả `p->gid` | ai cũng gọi được | không |
| 25 | `int setuid(int uid)` | đặt `p->uid` | chỉ root | không phải root; `uid < 0` |
| 26 | `int setgid(int gid)` | đặt `p->gid` | chỉ root | không phải root; `gid < 0` |
| 27 | `int chmod(char *path, int mode)` | đổi `ip->mode` | root, hoặc `p->uid == ip->uid` | không tìm thấy path; không đủ quyền |
| 28 | `int chown(char *path, int uid, int gid)` | đổi chủ và nhóm | chỉ root | không tìm thấy path; không phải root; `uid/gid < 0` |

Ghi chú:
- `setuid`/`setgid` đi một chiều: root hạ xuống user thường thì không lên lại được (không có euid/saved uid). Chủ ý để đơn giản.
- **Thứ tự bắt buộc trong `login`: `setgid` trước, `setuid` sau.** Sau `setuid` xuống user thường thì `setgid` bị từ chối.
- `chmod` chỉ giữ các bit hợp lệ của `mode` (hiện là `mode & 0777`; mở rộng khi làm P2).
- `argint` trả `void` trong bản xv6 này, nên việc kiểm tra giá trị âm phải tự làm sau khi lấy đối số.
- Mỗi syscall sửa 5 chỗ: `kernel/syscall.h`, dòng `extern` và bảng `syscalls[]` trong `kernel/syscall.c`, `user/usys.pl` (`entry("...")`), `user/user.h`.
- `getuid/getgid/setuid/setgid` cài trong `kernel/sysproc.c`. Với `chmod`/`chown`, spec này chốt chữ ký và quy tắc; phần cài đặt kernel (`sysfile.c`: `namei` → `ilock` → kiểm tra → sửa inode → `iupdate`, trong `begin_op/end_op`) làm cùng nhóm kiểm tra quyền.

## 5. Luồng init → login → setuid → sh

```
kernel khởi động: forkret → kexec("/init")
   └─ init (uid=0, gid=0)
        ├─ mở console làm fd 0,1,2 (mknod nếu chưa có)
        └─ vòng lặp: fork + exec "login"; wait()
              │
              ▼
           login (uid=0)
              1. in "login: ", đọc tên          (gets)
              2. in "password: ", đọc mật khẩu
              3. getpwnam(tên), so khớp mật khẩu
                   sai hoặc không có user → "Login incorrect", quay lại bước 1
              4. setgid(entry.gid)      ← trước
              5. setuid(entry.uid)      ← sau
              6. exec("sh")
              ▼
           sh (uid/gid của user) → lệnh con kế thừa qua kfork/kexec
              thoát sh (Ctrl-D) → login kết thúc → init chạy lại login
```

- Sửa `user/init.c`: `argv[] = {"login", 0}`, `exec("login", argv)`, thông báo cho khớp. Phần còn lại của `init` giữ nguyên.
- `init` chạy bằng root nên mở và tạo `console` như cũ; các fd 0,1,2 được kế thừa qua `kfork`.
- Đổi user bằng cách thoát shell rồi đăng nhập lại; lệnh `su` thuộc đề xuất P2.
- Chương trình người dùng: `login`; `whoami` (gọi `getuid()`, tra passwd, in tên; in số nếu không có dòng khớp); `chmod <mode bát phân> <file>` và `chown <user> <file>` gọi syscall tương ứng. Thêm vào `UPROGS` trong Makefile.
- Hạn chế đã biết: console xv6 tự hiện ký tự khi gõ nên mật khẩu hiện trên màn hình; không có cách tắt trong xv6 gốc.

## 6. Đề xuất 

## P1. Mật khẩu băm (stretch)

- Dòng passwd: `name:salt$hash:uid:gid`; chỉ đổi nội dung trường 2.
- `hash = hex(SHA-256(salt + mật khẩu))`, tự cài khoảng 100 dòng C theo FIPS 180-4 (`sha256.c`), tùy chọn lặp 1000 vòng. Salt là chuỗi hex 8 ký tự cố định cho từng user, ghi ngay trong file (xv6 không có nguồn ngẫu nhiên).
- Công cụ `tools/mkhash.c` chạy trên máy host, dùng chung `sha256.c` với `user/pwd.c`, để sinh dòng cho `etc/passwd`. Mật khẩu thuần của user demo chỉ ghi trong tài liệu.
- `login` và `su` dùng chung một hàm `pw_check(entry, input)`.
- Vấn đề: để hash trong `/etc/passwd` (ai cũng đọc được) hay tách `/etc/shadow`

## P2. Setuid và su (`su` là sản phẩm bắt buộc)

- Bit setuid là 04000 trong `mode`. Cần UGP-12 ghi rõ "9 bit quyền, bit 04000 dành cho setuid" và `chmod` mở rộng mask thành `mode & 04777`.
- `kexec`: ngay sau `ilock(ip)` lưu `suid = ip->mode & 04000` và `fuid = ip->uid` (vì `ip` bị thả trước bước commit của mã gốc); sau dòng `p->trapframe->sp = sp;` đặt `if (suid) p->uid = fuid;`. Như vậy exec thất bại không đổi uid.
- Vấn đề: Không thêm euid, nên `su` luôn hỏi mật khẩu của user đích, kể cả khi root gọi. `su` thuộc root, mode 04755. Phải làm thêm euid nếu cần đúng ngữ nghĩa Unix.
- Hạn chế: ghi vào file setuid không xóa bit setuid.

## P3. Đưa /etc/passwd vào ảnh đĩa

- `login` cần `/etc/passwd` có sẵn trong `fs.img`. mkfs gốc chỉ tạo thư mục gốc và `assert` từ chối tên có dấu `/`, nên cần tạo thư mục `etc`, ghi `etc/passwd` vào đó, và tăng `nlink` của root theo số thư mục con.
- Makefile: thêm `etc/passwd` vào cả phụ thuộc và lệnh của dòng `fs.img`; thiếu phụ thuộc thì sửa passwd mà `fs.img` không dựng lại.
- Mode và chủ sở hữu inode ban đầu do UGP-12 quyết định (mkfs khởi tạo metadata); riêng `passwd` đề xuất 0644, owner 0:0.

