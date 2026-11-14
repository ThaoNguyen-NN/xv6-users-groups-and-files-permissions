## Process identity (UGP-17)

Mỗi process có `uid` và `gid` (`uint`). Process đầu tiên (`init`) là root (uid 0, gid 0).
`fork` sao chép `uid/gid` của cha; `exec` giữ nguyên.

| Syscall | Số | Trả về | Lỗi (-1) |
|---|---|---|---|
| `int getuid(void)` | 23 | uid của process | không |
| `int getgid(void)` | 24 | gid của process | không |
| `int setuid(int uid)` | 25 | 0 | không phải root (uid hiện tại khác 0); `uid < 0` |
| `int setgid(int gid)` | 26 | 0 | không phải root (uid hiện tại khác 0); `gid < 0` |

Quy tắc:
- Chỉ root được gọi `setuid` và `setgid`. Việc đổi một chiều: sau khi hạ xuống user thường thì không đổi lại được.
- Khi hạ quyền phải gọi `setgid` trước, `setuid` sau, vì sau `setuid` xuống user thường thì `setgid` bị từ chối.
- Số 27 và 28 dành cho `chmod` và `chown`.

Chương trình: `whoami` in `uid=<n> gid=<n>` (sẽ in tên user khi có `/etc/passwd`); `uidtest` kiểm tra các quy tắc trên.
## File metadata (UGP-12)

### 1. Cấu trúc dữ liệu

Các trường metadata được bổ sung vào:

- `struct dinode` trong `kernel/fs.h`: lưu metadata trên đĩa.
- `struct inode` trong `kernel/file.h`: lưu metadata trong bộ nhớ.
- `struct stat` trong `kernel/stat.h`: cung cấp metadata cho chương trình người dùng.

`struct dinode` có kích thước 128 byte. Các trường `mode`, `uid`, `gid` được lưu cùng inode; vùng `reserved[13]` được giữ lại để bảo đảm kích thước đã thống nhất.

### 2. Ý nghĩa các trường

| Trường | Ý nghĩa |
|---|---|
| `mode` | Các bit quyền đọc, ghi, thực thi, biểu diễn bằng bát phân |
| `uid` | ID người sở hữu file |
| `gid` | ID nhóm sở hữu file |

### 3. API truy vấn metadata

| API | Chức năng |
|---|---|
| `int stat(const char *path, struct stat *st)` | Lấy metadata theo đường dẫn |
| `int fstat(int fd, struct stat *st)` | Lấy metadata từ file descriptor |

`struct stat` trả về các trường `mode`, `uid`, `gid` cùng thông tin inode hiện có.

### 4. Đồng bộ metadata

- `ialloc()`: khởi tạo inode mới.
- `iupdate()`: ghi metadata từ inode trong bộ nhớ xuống đĩa.
- `ilock()`: đọc metadata từ dinode trên đĩa vào inode trong bộ nhớ.
- `stati()`: sao chép metadata vào `struct stat`.

### 5. Giá trị mặc định

| Đối tượng | Mode | UID | GID |
|---|---:|---:|---:|
| Thư mục | 0755 | 0 | 0 |
| File dữ liệu tạo bởi mkfs | 0644 | 0 | 0 |
| Chương trình người dùng tạo bởi mkfs | 0755 | 0 | 0 |
| `/etc/passwd` | 0644 | 0 | 0 |
| Thiết bị console | 0666 | 0 | 0 |
| File mới tạo bởi process | 0644 | UID process | GID process |

Khi tạo inode mới:
- Thư mục nhận mode 0755.
- File thông thường nhận mode 0644.
- Thiết bị nhận mode 0666.
- UID và GID được lấy từ process hiện tại.

### 6. Kiểm thử

`metatest` kiểm tra metadata của `/README`, `/init`, `/etc`, `/etc/passwd`, `/console`, file mới và thư mục mới do root tạo, cùng file được tạo bởi process có UID 1000, GID 100.

Metadata được truy vấn bằng `stat()`.

Kết quả kiểm thử đã xác nhận:
- `metatest: ALL PASSED`.
- `usertests: ALL TESTS PASSED`.
- xv6 khởi động lại và tiếp tục đọc được metadata mà không cần tạo lại `fs.img`.
## Test dự kiến

Định dạng in: mỗi ca một dòng `PASS: <nội dung>` hoặc `FAIL: <nội dung>`, dòng cuối `<tên test>: ALL PASSED` hoặc `<tên test>: FAILED`.

- [x] Nguyên: `uidtest` kiểm tra `getuid/getgid/setuid/setgid` và việc kế thừa qua `fork/exec` (`user/uidtest.c`).
- [ ] Thư: `pwdtest` kiểm tra tra cứu user/group.
  - Cần: `user/pwd.c` (`getpwnam`, `getpwuid`) và `/etc/passwd` có trong `fs.img`.
  - Đạt khi: `root` cho uid 0 gid 0; `alice` cho 1000/100; `carol` cho 1001/100 (cùng gid với alice); `bob` cho 1002/200; tên không tồn tại và uid không tồn tại (ví dụ 9999) đều báo không tìm thấy; `getpwuid(1000)` trả về `alice`; dòng passwd dài vẫn đọc đúng khi bị cắt giữa hai lần `read`.
- [x] Đức Anh: `metatest` kiểm tra `mode/uid/gid` của file qua `stat`.
  - Cần: `struct stat` có `mode/uid/gid` và `stati()` sao chép các trường này.
  - Đạt khi (theo thiết kế UGP-12): file do mkfs tạo (`/README`) có mode 0644, uid 0, gid 0, còn chương trình (`/init`) có mode 0755; file mới do root tạo có mode 0644, thư mục mới 0755; `console` 0666; file tạo bởi process uid 1000 gid 100 mang đúng uid/gid đó; các giá trị còn nguyên sau khi khởi động lại qemu mà không dựng lại `fs.img`.

## User lookup API (UGP-18)

### Dữ liệu /etc/passwd

Định dạng mỗi dòng        name:password:uid:gid

Tài khoản mẫu:
- root: UID 0, GID 0
- alice: UID 1000, GID 100
- carol: UID 1001, GID 100
- bob: UID 1002, GID 200

### Các hàm

1. getpwnam(const char *name)
   - Tìm user theo tên.
   - Trả về con trỏ struct passwd nếu tìm thấy.
   - Trả NULL nếu không tìm thấy hoặc không đọc được file.

2. getpwuid(int uid)
   - Tìm user theo UID.
   - Trả về con trỏ struct passwd nếu tìm thấy.
   - Trả NULL nếu không tìm thấy hoặc lỗi.

3. getpwgid(int gid)
   - Tìm user đầu tiên có GID tương ứng.
   - Trả về NULL nếu không tìm thấy hoặc lỗi.

### Xử lý dữ liệu

- Đọc file bằng open(), read(), close().
- Ghép dữ liệu qua nhiều lần read().
- Phân tích các trường name, password, uid, gid.
- Bỏ qua dòng sai định dạng.
- Kiểm tra UID/GID là số nguyên hợp lệ.
- Giới hạn độ dài dòng bằng bộ đệm.

Kết quả tra cứu dùng vùng nhớ static,
có thể bị ghi đè bởi lần tra cứu thành công tiếp theo.

### Kiểm thử

Chương trình: user/pwdtest.c
Lệnh: pwdtest --fixture

Kết quả ngày 07/11/2026:
- 10 tests passed
- 0 tests failed
- pwdtest: ALL PASSED

Ghi chú:
Kiểm thử sử dụng dữ liệu fixture tạm.
Kiểm thử tích hợp với /etc/passwd trong fs.img sẽ thực hiện sau khi hoàn thành UGP-19.

## Permission Checking API (UGP-20)

Người thực hiện: Trần Thị Thư
Hàm check_permission() kiểm tra quyền truy cập
theo mô hình DAC: Owner - Group - Others.

### Khai báo

int check_permission(uint mode,
                     uint file_uid,
                     uint file_gid,
                     uint proc_uid,
                     uint proc_gid,
                     int requested);

### Quy ước

- PERM_READ = 4
- PERM_WRITE = 2
- PERM_EXEC = 1
- ALLOW = 1
- DENY = 0

### Kiểm thử

File: tests/permtest.c

Kết quả ngày 08/11/2026:
- 20 tests passed
- 0 tests failed
- permtest: ALL PASSED
Ghi chú:
Hàm được kiểm thử độc lập.
Chưa tích hợp vào open, unlink, chdir hoặc exec.
Việc tích hợp được thực hiện trong tuần 4.
### UGP-23 – File System Permission Enforcement

#### 1. API kiểm tra quyền

```c
int check_permission(uint mode, uint file_uid, uint file_gid,
                     uint proc_uid, uint proc_gid, int requested);
```

Hàm trả về `1` khi được phép truy cập và `0` khi bị từ chối. Các quyền được biểu diễn bằng `PERM_READ = 4`, `PERM_WRITE = 2`, `PERM_EXEC = 1`.

Trong `kernel/sysfile.c`, hàm hỗ trợ `has_permission()` lấy metadata từ inode và tiến trình để gọi `check_permission()`. Inode phải được khóa trước khi sử dụng.

#### 2. Quy tắc kiểm tra quyền

| System call | Quyền yêu cầu | Đối tượng kiểm tra |
|---|---|---|
| `sys_open` – `O_RDONLY` | READ | File |
| `sys_open` – `O_WRONLY` | WRITE | File |
| `sys_open` – `O_RDWR` | READ + WRITE | File |
| `sys_open` – `O_TRUNC` | Bổ sung WRITE | File |
| `sys_unlink` | WRITE + EXEC/SEARCH | Thư mục cha |
| `sys_chdir` | EXEC/SEARCH | Thư mục đích |

Khi không đủ quyền, syscall trả về `-1` và thực hiện giải phóng tài nguyên theo luồng xử lý lỗi hiện có.

Kiểm tra quyền tạo file mới trong `create()` thuộc UGP-25. Kiểm tra quyền thực thi chương trình và duyệt các thư mục trung gian thuộc UGP-24.

#### 3. Kiểm thử

- Biên dịch kernel bằng `make kernel/kernel`.
- Khởi động xv6 trên QEMU, kiểm tra các thao tác tạo, đọc, ghi, chuyển thư mục và xóa file.
- Chạy `usertests -q` để kiểm tra hồi quy.
- Sử dụng `user/ugp23test.c` để kiểm thử thao tác được phép và bị từ chối với user thường.

Kết quả kiểm thử thực tế được lưu trong `docs/weekly/W04/evidence/`. Các trường hợp chưa được kiểm thử không được tính là PASS.