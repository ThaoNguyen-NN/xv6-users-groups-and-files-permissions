# Design Proposal: User, Group and File Permissions on xv6-riscv

## 1. Overview

### 1.1. Mục tiêu

Dự án hướng đến mở rộng hệ điều hành xv6-riscv bằng cách xây dựng cơ chế quản lý người dùng (User), nhóm (Group) và phân quyền truy cập tệp (File Permissions).

Trong phiên bản xv6 ban đầu, hệ thống tệp chưa lưu thông tin chủ sở hữu và quyền truy cập của từng tệp. Các tiến trình cũng chưa có định danh người dùng hoặc nhóm. Vì vậy, hệ thống chưa thể phân biệt quyền truy cập giữa các user.

Thiết kế đề xuất bổ sung UID/GID cho mỗi process, mở rộng metadata của inode và xây dựng cơ chế kiểm tra quyền theo mô hình Unix DAC (Discretionary Access Control).

### 1.2. Phạm vi

Các chức năng chính bao gồm:

- Quản lý UID/GID trong tiến trình, hỗ trợ kế thừa qua `fork()` và duy trì qua `exec()`.
- Lưu thông tin tài khoản trong `/etc/passwd`, hỗ trợ tra cứu user/group.
- Lưu metadata `mode`, `uid`, `gid` trên filesystem.
- Kiểm tra quyền đọc (Read), ghi (Write), thực thi (Execute) theo Owner – Group – Others.
- Xử lý ngoại lệ đối với tài khoản Root.
- Cung cấp các syscall phục vụ truy vấn và thay đổi thông tin định danh, quyền truy cập.

## 2. Data Structures

### 2.1. Process Identity

Mở rộng `struct proc` trong `kernel/proc.h` bằng hai trường:

```c
uint uid;
uint gid;
```

Mỗi process có một cặp định danh UID/GID. Process đầu tiên được khởi tạo với `uid = 0`, `gid = 0` (Root).

- `allocproc()`: khởi tạo UID/GID mặc định bằng 0.
- `kfork()`: tiến trình con kế thừa UID/GID từ tiến trình cha.
- `kexec()`: giữ nguyên UID/GID hiện tại.

Thiết kế hiện tại sử dụng một GID chính cho mỗi process, chưa hỗ trợ supplementary groups.

### 2.2. File Metadata

Bổ sung các trường `mode`, `uid`, `gid` vào ba cấu trúc:

- `struct dinode` (`kernel/fs.h`): lưu metadata trên đĩa.
- `struct inode` (`kernel/file.h`): lưu metadata trong bộ nhớ.
- `struct stat` (`kernel/stat.h`): cung cấp metadata cho chương trình user space.

Cấu trúc dinode được thống nhất ở kích thước 128 byte:

```c
struct dinode {
  short type;
  short major;
  short minor;
  short nlink;

  uint mode;
  uint uid;
  uint gid;

  uint size;
  uint addrs[NDIRECT + 1];

  uint reserved[13];
};
```

Với `BSIZE = 1024`, mỗi block chứa:

`IPB = 1024 / 128 = 8 inode`

Thay đổi này làm tăng số block dành cho inode trong filesystem. Các hàm `ialloc()`, `ilock()`, `iupdate()` và `stati()` phải đồng bộ đầy đủ metadata giữa đĩa, bộ nhớ và `struct stat`.

### 2.3. User and Group Database

Sử dụng `/etc/passwd` để lưu thông tin tài khoản theo định dạng:

```text
name:password:uid:gid
```

Các tài khoản mẫu:

| User | Password | UID | GID |
|---|---|---:| ---:|
| root | root123 | 0 | 0 |
| alice | alice123 | 1000 | 100 |
| carol | carol123 | 1001 | 100 |
| bob | bob123 | 1002 | 200 |

Các user có cùng GID được xem là thuộc cùng một nhóm. Thiết kế hiện tại chưa sử dụng tệp `/etc/group`.

Trong user space, `user/pwd.c` cung cấp các hàm `getpwnam()`, `getpwuid()` và `getpwgid()` để tra cứu thông tin. Việc đọc dữ liệu sử dụng `open()`, `read()`, `close()`, đồng thời xử lý trường hợp một dòng bị chia giữa nhiều lần đọc.

### 2.4. Permission Model

Quyền truy cập được biểu diễn bằng 9 bit theo chuẩn Unix:

| Permission | Giá trị | Ý nghĩa |
|---|---:|---|
| Read (r) | 4 | Đọc nội dung tệp |
| Write (w) | 2 | Ghi hoặc thay đổi nội dung |
| Execute (x) | 1 | Thực thi tệp hoặc tìm kiếm thư mục |

Ví dụ:
- `0600`: Chỉ Owner được đọc và ghi.
- `0644`: Owner được đọc và ghi; Group và Others chỉ được đọc.
- `0755`: Owner có đầy đủ quyền; Group và Others được đọc, thực thi.

Hàm `check_permission()` trong `kernel/perm.c` nhận metadata của inode, UID/GID của process và quyền được yêu cầu. Thuật toán lựa chọn lớp quyền theo thứ tự:

1. Root: được bỏ qua kiểm tra DAC thông thường; riêng Execute vẫn yêu cầu có ít nhất một bit thực thi.
2. Owner: UID của process trùng UID của tệp.
3. Group: GID của process trùng GID của tệp.
4. Others: các trường hợp còn lại.

Chỉ khi toàn bộ quyền được yêu cầu đều được đáp ứng, thao tác mới được ALLOW.

## 3. Files Touched

### 3.1. Kernel files

| File | Thay đổi |
|---|---|
| `kernel/proc.h` | Thêm UID/GID vào process |
| `kernel/proc.c` | Khởi tạo và kế thừa UID/GID |
| `kernel/fs.h` | Mở rộng `struct dinode` |
| `kernel/file.h` | Mở rộng `struct inode` |
| `kernel/stat.h` | Bổ sung metadata vào `struct stat` |
| `kernel/fs.c` | Đồng bộ metadata qua ialloc, ilock, iupdate, stati |
| `kernel/sysproc.c` | Xử lý syscall UID/GID |
| `kernel/syscall.h` | Khai báo số syscall |
| `kernel/syscall.c` | Đăng ký syscall |
| `kernel/defs.h` | Khai báo các hàm kernel |
| `kernel/perm.c`, `kernel/perm.h` | Xây dựng cơ chế kiểm tra quyền |
| `kernel/sysfile.c` | Khởi tạo metadata và tích hợp kiểm tra quyền |
| `kernel/exec.c` | Tích hợp kiểm tra quyền Execute |

### 3.2. User space and filesystem

| File | Thay đổi |
|---|---|
| `user/user.h` | Khai báo syscall và API |
| `user/usys.pl` | Sinh syscall stub |
| `user/pwd.c`, `user/pwd.h` | Tra cứu user/group |
| `user/whoami.c` | Hiển thị UID/GID |
| `user/uidtest.c` | Kiểm tra process identity |
| `user/pwdtest.c` | Kiểm tra tra cứu tài khoản |
| `user/metatest.c` | Kiểm tra metadata inode |
| `tests/permtest.c` | Kiểm thử độc lập thuật toán phân quyền |
| `etc/passwd` | Lưu dữ liệu tài khoản mẫu |
| `mkfs/mkfs.c` | Khởi tạo metadata và đưa passwd vào fs.img |
| `Makefile` | Bổ sung object, chương trình và dữ liệu filesystem |

Các file liên quan đến kiểm tra quyền trong `sysfile.c` và `exec.c` cần được hoàn thiện để bảo đảm mọi thao tác truy cập đều tuân theo cùng một chính sách.

## 4. New System Calls

Các syscall được thiết kế theo bảng sau:

| Syscall | Number | Chức năng | Quyền thực hiện |
|---|---:|---|---|
| `getuid()` | 23 | Lấy UID hiện tại | Mọi process |
| `getgid()` | 24 | Lấy GID hiện tại | Mọi process |
| `setuid(uid)` | 25 | Thay đổi UID | Chỉ Root |
| `setgid(gid)` | 26 | Thay đổi GID | Chỉ Root |
| `chmod(path, mode)` | 27 | Thay đổi quyền tệp | Owner hoặc Root |
| `chown(path, uid, gid)` | 28 | Thay đổi chủ sở hữu | Chỉ Root |

Bốn syscall đầu đã được triển khai và đăng ký trong hệ thống. `chmod()` và `chown()` là các syscall được đặc tả cho phần mở rộng tiếp theo, chưa có trong bảng syscall hiện tại.

Một số quy tắc cần bảo đảm:

- `setuid()` và `setgid()` chỉ cho phép Root thay đổi UID/GID.
- Việc hạ quyền là một chiều; process đã trở thành user thường không thể tự nâng quyền trở lại.
- Khi chuyển sang user thường, phải gọi `setgid()` trước `setuid()`.
- `chmod()` chỉ cho phép Owner hoặc Root thay đổi mode.
- `chown()` chỉ cho phép Root thay đổi UID/GID của tệp.

## 5. Permission Integration

Hàm `check_permission()` cần được tích hợp vào các điểm truy cập filesystem quan trọng:

| Kernel function | Quyền yêu cầu |
|---|---|
| `sys_open()` | Read/Write theo chế độ mở |
| `sys_chdir()` | Execute/Search trên thư mục đích |
| `sys_unlink()` | Write + Search trên thư mục cha |
| `kexec()` | Execute trên file chương trình |
| `namex()` | Search trên từng thư mục trong đường dẫn |

Đặc biệt, `O_RDONLY = 0` nên không thể kiểm tra bằng phép AND thông thường. Với `O_TRUNC`, kernel phải xác nhận quyền Write trước khi gọi `itrunc()`.

Đối với `O_CREATE`, cần kiểm tra Write và Search trên thư mục cha trước khi tạo directory entry. Tương tự, `unlink()` phải kiểm tra quyền trên thư mục cha thay vì chỉ kiểm tra quyền của file đích.

## 6. Test Plan

Các bài kiểm thử được chia thành bốn nhóm chính.

| Test | Nội dung | Tiêu chí PASS |
|---|---|---|
| `uidtest` | getuid/getgid, setuid/setgid, fork/exec | UID/GID đúng và được kế thừa |
| `pwdtest` | Tra cứu user theo tên, UID, GID | Trả đúng tài khoản, xử lý dữ liệu lỗi |
| `metatest` | Kiểm tra mode, uid, gid qua stat | Metadata chính xác và được lưu bền vững |
| `permtest` | Kiểm tra Owner, Group, Others và Root | Kết quả ALLOW/DENY đúng chính sách |

### 6.1. Permission Test Matrix

Các tình huống quan trọng:

- Owner được phép truy cập khi bit quyền tương ứng được cấp.
- Group được phép truy cập theo quyền của nhóm.
- Others chỉ được truy cập trong phạm vi quyền còn lại.
- Owner không được chuyển sang quyền Group hoặc Others khi thiếu quyền Owner.
- Root được bỏ qua kiểm tra Read/Write thông thường nhưng phải đáp ứng điều kiện Execute.
- Các yêu cầu quyền không hợp lệ phải bị từ chối.

### 6.2. Integration Tests

Sau khi tích hợp kiểm tra quyền vào kernel, cần bổ sung các tình huống:

1. Alice tạo tệp với mode `0600`; Bob không được đọc hoặc ghi.
2. Carol cùng nhóm với Alice được truy cập theo quyền Group.
3. Bob thuộc nhóm khác chỉ được truy cập theo quyền Others.
4. User không có quyền Execute không thể chạy chương trình.
5. User không có quyền Search không thể duyệt thư mục.
6. User không có quyền Write + Search không thể xóa tệp trong thư mục.
7. Root thực hiện được các thao tác quản trị theo chính sách đã xác định.

### 6.3. Acceptance Criteria

Dự án được xem là đạt yêu cầu khi:

- `make fs.img` hoàn thành thành công.
- xv6 khởi động ổn định trên QEMU.
- Metadata mode/uid/gid được ghi và đọc lại chính xác, kể cả sau khi khởi động lại.
- Các syscall trả về đúng kết quả theo quy tắc phân quyền.
- Các thao tác filesystem từ chối truy cập trái phép.
- Các bài test riêng lẻ và `usertests` đều đạt yêu cầu.

## 7. Expected Outcome

Sau khi hoàn thiện, xv6-riscv có thể quản lý danh tính người dùng và nhóm, lưu thông tin chủ sở hữu của từng tệp và kiểm soát truy cập theo mô hình Owner – Group – Others.

Thiết kế cũng tạo nền tảng để phát triển thêm các tiện ích quản trị như `chmod`, `chown` và cơ chế đăng nhập người dùng trong những giai đoạn tiếp theo.
