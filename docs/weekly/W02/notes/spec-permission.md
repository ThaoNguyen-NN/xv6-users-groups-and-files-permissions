# UGP – Permission Specification (Week 02)

## 1. Mục tiêu và phạm vi
Mục tiêu:
- Xây dựng quy tắc Owner – Group – Others.
- Kiểm tra quyền Read, Write, Execute/Search.
- Xác định ngoại lệ của tài khoản Root.
- Xác định vị trí tích hợp kiểm tra quyền vào các hàm kernel.
- Đặc tả các trường hợp mở file đặc biệt.
- Xây dựng thuật toán `check_permission()` làm cơ sở triển khai.

Tài liệu mô tả thiết kế dự kiến, không khẳng định các chức năng đã được lập trình.

## 2. Hiện trạng mã nguồn xv6

Qua phân tích:

- `kernel/proc.h`: `struct proc` chưa có UID và GID.
- `kernel/fs.h`: `struct dinode` chưa có UID, GID và mode.
- `kernel/file.h`: `struct inode` chưa có UID, GID và mode.
- `kernel/sysfile.c`: chưa thực hiện kiểm tra quyền DAC theo UID/GID/mode trong các thao tác được xem xét.
- `kernel/exec.c`: `kexec()` chưa kiểm tra quyền Execute theo Owner/Group/Others.
- `kernel/fs.c`: `namex()` chưa kiểm tra quyền Search trên từng thư mục.

Đề xuất: 

| File | Cấu trúc | Trường cần có |
|---|---|---|
| proc.h | struct proc | uid, gid |
| fs.h | struct dinode | uid, gid, mode |
| file.h | struct inode | uid, gid, mode |

Các thay đổi cấu trúc inode trên đĩa cần được đồng bộ với `mkfs`, `ilock()` và `iupdate()`.

## 3. Quy tắc Owner – Group – Others

### 3.1. Các loại quyền

| Quyền | Bit | Giá trị | File | Thư mục |
|---|---|---|---|---|
| Read | r | 4 | Đọc nội dung | Liệt kê entry |
| Write | w | 2 | Ghi nội dung | Thay đổi entry, kết hợp các điều kiện liên quan |
| Execute | x | 1 | Thực thi | Search/traverse |

### 3.2. Quy tắc phân quyền

Hệ thống sử dụng mô hình DAC (Discretionary Access Control) dựa trên UID, GID và mode của inode.

Thứ tự lựa chọn:
1. Kiểm tra ngoại lệ root.
2. Nếu UID process trùng UID inode: chọn Owner.
3. Nếu process thuộc nhóm inode: chọn Group.
4. Ngược lại: chọn Others.

Sau khi chọn lớp quyền, hệ thống so sánh các bit được cấp với các bit mà thao tác yêu cầu.
Nếu đủ toàn bộ quyền yêu cầu: ALLOW.
Nếu thiếu bất kỳ quyền nào: DENY.

### 3.3. Cấu trúc mode

Các bit quyền:

- Owner: `(mode >> 6) & 7`
- Group: `(mode >> 3) & 7`
- Others: `mode & 7`

### 3.4. Ngoại lệ Root

Đề xuất:

- UID 0 được xem là Root trong mô hình đơn giản.
- Root bỏ qua kiểm tra DAC Read/Write thông thường.
- Đối với Execute trên file thông thường, yêu cầu có ít nhất một bit Execute.
- Root không bỏ qua kiểm tra tính hợp lệ của loại inode và lỗi hệ thống tệp.

Chính sách này cần được nhóm xác nhận trước khi triển khai.

## 4. Vị trí kiểm tra quyền trong Kernel

| Hàm | File | Quyền cần kiểm tra | Đối tượng |
|---|---|---|---|
| sys_open | sysfile.c | R/W theo cờ | File |
| sys_chdir | sysfile.c | X | Thư mục đích |
| sys_unlink | sysfile.c | W + X | Thư mục cha |
| kexec | exec.c | X | File thực thi |
| namex | fs.c | X/Search | Mỗi thư mục duyệt |
| dirlookup | fs.c | X/Search theo ngữ cảnh | Thư mục được tra cứu |

### 4.1. sys_open()

- Mở file đã tồn tại: kiểm tra quyền theo chế độ mở sau khi khóa inode và trước khi cấp phát file descriptor hoặc thay đổi nội dung.
- Mở file mới: kiểm tra quyền tạo entry trong thư mục cha trước khi thực hiện tạo.
- Không thực hiện `itrunc()` trước khi xác nhận quyền Write.

### 4.2. sys_chdir()

- Kiểm tra inode đích là thư mục.
- Kiểm tra quyền Execute/Search của thư mục đích trước khi cập nhật `cwd`.
- Nếu không có quyền, trả về lỗi và giữ nguyên thư mục làm việc.

### 4.3. sys_unlink()

- Kiểm tra Write + Search trên thư mục cha.
- Kiểm tra trước khi xóa directory entry.
- Không quyết định quyền xóa chỉ dựa vào các bit Write của file đích.

### 4.4. kexec()

- Kiểm tra Execute của file chương trình trước khi đọc và nạp ELF.
- Từ chối thực thi nếu không đủ quyền.
- Các quyền trang bộ nhớ PTE_X/PTE_W không thay thế quyền Execute của file.

### 4.5. namex()/dirlookup()

- Khi phân giải đường dẫn, kiểm tra Search trên mỗi thư mục phải duyệt.
- Đảm bảo trường hợp nameiparent() cũng kiểm tra đúng thư mục cha.
- Ưu tiên đặt kiểm tra theo ngữ cảnh tại namex() và các system call.
- Không đặt kiểm tra quyền tiến trình vô điều kiện tại dirlookup() khi chưa rà soát các lời gọi nội bộ.

## 5. Xử lý các cờ mở file

| Cờ | Quy tắc |
|---|---|
| O_RDONLY | Yêu cầu Read |
| O_WRONLY | Yêu cầu Write |
| O_RDWR | Yêu cầu Read + Write |
| O_TRUNC | Yêu cầu thêm Write trước khi cắt ngắn file |
| O_CREATE | Nếu tạo file mới, yêu cầu Write + Search trên thư mục cha |

Lưu ý:

`O_RDONLY = 0` nên không được kiểm tra bằng `omode & O_RDONLY`.

Nếu `O_CREATE` áp dụng cho file đã tồn tại, cần kiểm tra quyền mở file hiện có theo chế độ truy cập, không tự động coi là file mới.

Những thao tác làm thay đổi nội dung phải thực hiện sau khi quyền đã được xác nhận.

## 6. Pseudo-code check_permission()

Quy ước:
- READ = 4
- WRITE = 2
- EXEC = 1

```text
check_permission(process, inode, requested):

    uid = process.uid
    gid = process.gid
    mode = inode.mode

    if uid == 0:
        if requested contains EXEC
           and inode is a regular file
           and (mode & 0111) == 0:
            return DENY
        return ALLOW

    if uid == inode.uid:
        allowed = (mode >> 6) & 7
    else if gid == inode.gid:
        allowed = (mode >> 3) & 7
    else:
        allowed = mode & 7

    if (allowed & requested) == requested:
        return ALLOW

    return DENY
```



## 7. Permission Rules Table - Bảng quy tắc kiểm tra quyền

| Operation | Target | Required |
|---|---|---|
| Open read-only | File | R |
| Open write-only | File | W |
| Open read-write | File | R + W |
| Open truncate | File | W + quyền mở tương ứng |
| Create file | Parent directory | W + X |
| Chdir | Directory | X |
| Unlink | Parent directory | W + X |
| Exec | File | X |
| Path lookup | Traversed directories | X |

Chmod (để nhất quán với test matrix tuần 1): đề xuất chỉ Owner hoặc Root được phép thay đổi mode.

