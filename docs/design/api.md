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
