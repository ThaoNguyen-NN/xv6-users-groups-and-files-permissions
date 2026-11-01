# Chốt bố cục mode, uid, gid trong inode xv6

**Trạng thái:** Đề xuất thiết kế, chờ nhóm review và thống nhất.

## 1. Mục tiêu

Bổ sung thông tin quyền truy cập và chủ sở hữu vào inode của xv6, phục vụ cơ chế phân quyền theo người dùng và nhóm.

Ba trường được bổ sung:

- `mode`: Quyền đọc, ghi và thực thi.
- `uid`: Mã định danh người dùng sở hữu tệp.
- `gid`: Mã định danh nhóm sở hữu tệp.

## 2. Bố cục dinode trên đĩa

Đề xuất sử dụng kiểu `uint` cho ba trường mới và bổ sung vùng dự trữ để kích thước `dinode` đạt 128 byte.

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

## 3. Kích thước và vị trí các trường

| Trường | Kích thước | Vị trí byte |
|---|---:|---:|
| `type` | 2 byte | 0–1 |
| `major` | 2 byte | 2–3 |
| `minor` | 2 byte | 4–5 |
| `nlink` | 2 byte | 6–7 |
| `mode` | 4 byte | 8–11 |
| `uid` | 4 byte | 12–15 |
| `gid` | 4 byte | 16–19 |
| `size` | 4 byte | 20–23 |
| `addrs[13]` | 52 byte | 24–75 |
| `reserved[13]` | 52 byte | 76–127 |

Tổng kích thước: 128 byte.

Vùng `reserved[13]` được khởi tạo bằng 0 và chưa mang ý nghĩa phân quyền.

## 4. Bố cục inode trong bộ nhớ

Trong `kernel/file.h`, đề xuất bổ sung các trường tương ứng vào `struct inode`.

```c
struct inode {
  uint dev;
  uint inum;
  int ref;
  struct sleeplock lock;
  int valid;

  short type;
  short major;
  short minor;
  short nlink;

  uint mode;
  uint uid;
  uint gid;

  uint size;
  uint addrs[NDIRECT + 1];
};
```

Không cần thêm `reserved[13]` vào inode trong bộ nhớ vì vùng dự trữ chỉ phục vụ bố cục dữ liệu trên đĩa.

## 5. Ảnh hưởng đến bố cục hệ thống tệp

Kích thước block:

`BSIZE = 1024 byte`

Với dinode 64 byte ban đầu:

`IPB = 1024 / 64 = 16`

Với dinode 128 byte theo đề xuất:

`IPB = 1024 / 128 = 8`

Như vậy, mỗi block chứa 8 inode thay vì 16 inode.

Với `NINODES = 200`, theo công thức hiện tại của mkfs:

`ninodeblocks = NINODES / IPB + 1`

Số block inode dự kiến tăng từ 13 lên 26.

Công thức `IBLOCK` vẫn giữ nguyên:

`IBLOCK(i, sb) = i / IPB + sb.inodestart`

Vị trí inode trong block được xác định bằng `i % IPB`.

## 6. Quy ước quyền truy cập

Trường `mode` sử dụng 9 bit quyền cơ bản:

- `r`: Đọc.
- `w`: Ghi.
- `x`: Thực thi.

Các nhóm quyền:

- Owner: Quyền của chủ sở hữu.
- Group: Quyền của nhóm sở hữu.
- Other: Quyền của người dùng khác.

Các mức quyền được sử dụng trong kế hoạch kiểm thử:

| MODE | Owner | Group | Other |
|---|---|---|---|
| `0600` | `rw-` | `---` | `---` |
| `0640` | `rw-` | `r--` | `---` |
| `0644` | `rw-` | `r--` | `r--` |
| `0755` | `rwx` | `r-x` | `r-x` |

Chính sách Root theo bảng kiểm thử:

- Root có thể đọc và ghi mà không phụ thuộc các bit `r`, `w`.
- Root được thực thi khi ít nhất một trong ba bit `x` được bật.
- Chỉ Owner hoặc Root được thay đổi quyền.
- Chỉ Root được thay đổi chủ sở hữu.

## 7. Giá trị khởi tạo đề xuất

- Tệp dữ liệu thông thường: `0644`.
- Thư mục mới: `0755`.
- UID và GID của tệp mới: lấy từ tiến trình tạo tệp.
- Các inode ban đầu do mkfs tạo: đề xuất UID/GID bằng 0, cần nhóm thống nhất.

Chương trình thực thi được đưa vào ảnh hệ thống tệp phải được cấp quyền thực thi phù hợp.

## 8. Những thành phần cần cập nhật

- `kernel/fs.h`: Bổ sung trường vào `struct dinode`.
- `kernel/file.h`: Bổ sung trường vào `struct inode`.
- `kernel/fs.c`: Cập nhật quá trình khởi tạo, đọc và ghi metadata.
- `mkfs/mkfs.c`: Khởi tạo metadata và bảo đảm bố cục inode phù hợp.
- `kernel/proc.h`, `kernel/proc.c`: Quản lý UID/GID của tiến trình.
- `kernel/sysfile.c`, `kernel/exec.c`: Kiểm tra quyền truy cập.

## 9. Kết luận

Phương án dinode 128 byte được đề xuất để bổ sung metadata phân quyền mà vẫn duy trì nguyên tắc bố trí inode theo từng block của xv6.

Đây là thiết kế đang chờ nhóm review. Cần thống nhất kích thước, vùng dự trữ và chính sách khởi tạo trước khi sửa mã nguồn.
