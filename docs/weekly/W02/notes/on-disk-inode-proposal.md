# Đề xuất thiết kế dữ liệu inode lưu trên đĩa trong xv6

**Trạng thái:** Bản nháp đề xuất, chờ nhóm xem xét.

## 1. Mục tiêu

Đề xuất thay đổi cấu trúc inode lưu trên đĩa (`struct dinode`) để bổ sung thông tin quyền truy cập và chủ sở hữu tệp.

Ba trường cần bổ sung gồm:

- `mode`: Quyền đọc, ghi và thực thi.
- `uid`: Mã định danh người dùng sở hữu tệp.
- `gid`: Mã định danh nhóm sở hữu tệp.

Mục tiêu là cung cấp đủ metadata để triển khai cơ chế phân quyền theo người dùng và nhóm trong xv6.

## 2. Cấu trúc hiện tại

Trong `kernel/fs.h`, `struct dinode` hiện có các trường:

- `type`: Loại tệp hoặc đối tượng.
- `major`: Số thiết bị chính.
- `minor`: Số thiết bị phụ.
- `nlink`: Số liên kết.
- `size`: Kích thước tệp.
- `addrs`: Địa chỉ các khối dữ liệu.

Với `BSIZE = 1024` byte và kích thước dinode ban đầu bằng 64 byte:

`IPB = 1024 / 64 = 16`

Như vậy, mỗi block chứa được 16 inode.

Cấu trúc hiện tại chưa có các trường lưu quyền truy cập và thông tin chủ sở hữu.

## 3. Cấu trúc đề xuất

Đề xuất bổ sung ba trường `mode`, `uid`, `gid` vào `struct dinode`.

Để bảo đảm kích thước inode chia hết cho kích thước block theo điều kiện hiện có trong `mkfs.c`, đề xuất sử dụng thêm vùng dự trữ `reserved[13]`.

Cấu trúc đề xuất:

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

Đây là phương án thiết kế đang được đề xuất, chưa phải cấu trúc đã triển khai trong kernel.

## 4. Ý nghĩa các trường mới

### 4.1. Trường mode

Lưu quyền đọc, ghi và thực thi của ba nhóm:

- Chủ sở hữu.
- Nhóm sở hữu.
- Những người dùng khác.

Sử dụng chín bit quyền cơ bản theo cách biểu diễn hệ bát phân.

Ví dụ:

- `0600`: Chủ sở hữu được đọc và ghi.
- `0640`: Chủ sở hữu được đọc và ghi, nhóm chỉ được đọc.
- `0644`: Chủ sở hữu được đọc và ghi, nhóm và người khác chỉ được đọc.
- `0755`: Chủ sở hữu có toàn bộ quyền, nhóm và người khác được đọc và thực thi.

Trường `mode` chỉ lưu quyền truy cập, không thay thế trường `type` đang có.

### 4.2. Trường uid

Lưu mã định danh của người dùng sở hữu tệp.

Ví dụ, nếu Alice có UID 1001 và tạo một tệp mới, UID của tệp được đề xuất khởi tạo bằng 1001.

### 4.3. Trường gid

Lưu mã định danh của nhóm sở hữu tệp.

Ví dụ, nếu Alice thuộc nhóm có GID 100, GID của tệp được đề xuất khởi tạo bằng 100.

## 5. Bố cục dữ liệu trên đĩa

Đề xuất sử dụng kiểu `uint` 4 byte cho cả ba trường mới.

Bố cục byte:

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

Tổng kích thước đề xuất: 128 byte.

Vùng `reserved[13]` được sử dụng để giữ kích thước inode ở mức 128 byte, chưa mang ý nghĩa phân quyền.

Các phần tử dự trữ cần được khởi tạo về 0 khi tạo inode.

## 6. Ảnh hưởng đến bố cục hệ thống tệp

Kích thước block vẫn giữ nguyên:

`BSIZE = 1024 bytes`

Sau khi thay đổi kích thước dinode:

`IPB = 1024 / 128 = 8`

Như vậy, mỗi block chứa được 8 inode thay vì 16 inode như trước.

Với `NINODES = 200`, công thức hiện tại của mkfs:

`ninodeblocks = NINODES / IPB + 1`

Số block inode dự kiến tăng từ 13 lên 26.

Điều kiện hiện có:

`BSIZE % sizeof(struct dinode) == 0`

vẫn thỏa mãn vì 1024 chia hết cho 128.

Công thức `IBLOCK` tiếp tục sử dụng `IPB` để xác định block chứa inode. Vị trí inode bên trong block tiếp tục được xác định thông qua phép chia lấy dư.

## 7. Ảnh hưởng đến mkfs

Chương trình `mkfs/mkfs.c` lấy định nghĩa `struct dinode` từ `kernel/fs.h`.

Khi thay đổi cấu trúc, cần xem xét:

- Khởi tạo các trường `mode`, `uid`, `gid`.
- Khởi tạo vùng `reserved` bằng 0.
- Sử dụng đúng quy ước chuyển đổi thứ tự byte khi ghi các trường mới.
- Bảo đảm `winode()` và `rinode()` làm việc với cấu trúc inode mới.
- Kiểm tra lại số block dành cho vùng inode.

Các giá trị khởi tạo cần được thống nhất:

- Thư mục: đề xuất `0755`.
- Tệp dữ liệu thông thường: đề xuất `0644`.
- Chương trình thực thi: đề xuất `0755`.
- Các inode ban đầu do mkfs tạo: đề xuất UID/GID bằng 0, tùy chính sách của nhóm.

## 8. Tương thích với ảnh hệ thống tệp cũ

Thay đổi kích thước dinode làm thay đổi cách bố trí inode trên đĩa.

Vì vậy, ảnh hệ thống tệp được tạo bằng cấu trúc cũ không nên được sử dụng trực tiếp với cấu trúc mới.

Sau khi nhóm thống nhất và triển khai thiết kế, cần tạo lại ảnh hệ thống tệp bằng phiên bản `mkfs` tương ứng với kernel mới.

## 9. Những vấn đề cần nhóm thống nhất

1. Có chấp nhận phương án dinode 128 byte và vùng dự trữ 52 byte không?
2. Có sử dụng `uint` cho cả ba trường `mode`, `uid`, `gid` không?
3. Các inode ban đầu trong ảnh hệ thống tệp được gán UID/GID như thế nào?
4. Quyền mặc định cho từng loại tệp và thư mục là gì?
5. Có cần mở rộng thông tin inode được cung cấp qua cấu trúc `stat` không?
6. Cách tạo lại và kiểm thử ảnh hệ thống tệp sau khi thay đổi là gì?

## 10. Kết luận

Đề xuất bổ sung `mode`, `uid`, `gid` vào `struct dinode` nhằm cung cấp metadata cần thiết cho cơ chế phân quyền theo người dùng và nhóm.

Phương án 128 byte được đưa ra để duy trì cách bố trí inode theo từng block và đáp ứng điều kiện chia hết kích thước block trong chương trình `mkfs`.

Đây là bản nháp để các thành viên thảo luận và thống nhất trước khi chỉnh sửa mã nguồn của kernel và chương trình tạo ảnh hệ thống tệp.
