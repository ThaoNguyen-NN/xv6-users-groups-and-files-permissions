# Ghi chú khảo sát inode trong xv6 — Tuần 1

## 1. Mục tiêu

Tìm hiểu cách xv6 mô tả inode, lưu metadata trên đĩa, quản lý
inode trong bộ nhớ và tạo ảnh hệ thống tệp ban đầu.

## 2. Tệp kernel/fs.h

### Cấu trúc dinode

`struct dinode` mô tả inode được lưu trên đĩa.

Các trường hiện có:

- `type`: Loại tệp hoặc đối tượng.
- `major`: Số thiết bị chính.
- `minor`: Số thiết bị phụ.
- `nlink`: Số liên kết đến inode.
- `size`: Kích thước nội dung tệp, tính bằng byte.
- `addrs`: Các địa chỉ khối dữ liệu của tệp.

### Kích thước khối BSIZE

Trong mã nguồn, `BSIZE` bằng 1024 byte, tương đương 1 KiB.

### Số inode trong mỗi khối IPB

Công thức:

`IPB = BSIZE / sizeof(struct dinode)`

Với cấu trúc dinode ban đầu có kích thước 64 byte:

`IPB = 1024 / 64 = 16`

Như vậy, một khối chứa được 16 inode theo bố cục hiện tại.

### Chức năng của IBLOCK

Công thức:

`IBLOCK(i, sb) = (i / IPB) + sb.inodestart`

Công thức xác định khối trên đĩa chứa inode có số hiệu `i`.

- `i`: Số hiệu inode.
- `IPB`: Số inode trong một khối.
- `sb.inodestart`: Vị trí bắt đầu vùng lưu inode.

Phép chia nguyên `i / IPB` xác định nhóm inode, còn `i % IPB`
xác định vị trí của inode bên trong khối.

## 3. Cấu trúc inode trong kernel/file.h

`struct inode` là cấu trúc inode được kernel sử dụng trong RAM.

Các trường đã tìm thấy:

- `dev`: Thiết bị chứa inode.
- `inum`: Số hiệu inode.
- `ref`: Số tham chiếu đến inode trong bộ nhớ.
- `lock`: Khóa bảo vệ dữ liệu inode.
- `valid`: Cho biết metadata đã được nạp từ đĩa hay chưa.
- `type`, `major`, `minor`, `nlink`, `size`, `addrs`: Metadata
  được giữ trong bộ nhớ để kernel sử dụng.

`dinode` là bản lưu trữ trên đĩa, trong khi `inode` là bản
kernel sử dụng trong bộ nhớ. Hai cấu trúc có liên quan nhưng
không hoàn toàn giống nhau.

## 4. Tệp kernel/fs.c

Các hàm cần khảo sát trực tiếp:

- `ialloc()`: Tìm và cấp phát inode trong hệ thống tệp.
- `ilock()`: Khóa inode và nạp metadata từ đĩa vào bộ nhớ
  khi cần.
- `iupdate()`: Ghi metadata từ inode trong bộ nhớ xuống đĩa.
- `stati()`: Sao chép thông tin inode được hỗ trợ vào cấu trúc stat.

Cần đối chiếu mã nguồn thực tế để xác nhận cách từng hàm hoạt động
và xác định chính xác những vị trí cần cập nhật khi thêm metadata.

## 5. Tệp mkfs/mkfs.c

Chương trình mkfs tạo ảnh hệ thống tệp ban đầu và lấy định nghĩa
`struct dinode` từ `kernel/fs.h`.

### Hàm ialloc()

Hàm `ialloc()` trong mkfs cấp phát inode để xây dựng ảnh hệ thống
tệp. Hàm khởi tạo cấu trúc dinode, đặt các trường ban đầu, gán
loại inode và gọi `winode()` để ghi inode vào ảnh.

Hàm này khác với `ialloc()` trong kernel/fs.c: một hàm chạy khi
tạo ảnh hệ thống tệp, hàm còn lại phục vụ hệ thống tệp khi xv6
đang hoạt động.

### Hàm winode()

Hàm `winode()` ghi inode vào ảnh hệ thống tệp:

1. Dùng `IBLOCK()` tìm khối chứa inode.
2. Đọc khối vào bộ đệm bằng `rsect()`.
3. Dùng `inum % IPB` tìm vị trí inode trong khối.
4. Sao chép dữ liệu inode vào bộ đệm.
5. Ghi khối đã cập nhật bằng `wsect()`.

### Hàm rinode()

Hàm `rinode()` đọc inode từ ảnh hệ thống tệp. Cần xem phần
cài đặt đầy đủ để ghi lại chính xác quá trình đọc.

### Ảnh hưởng đến bố cục đĩa

mkfs có kiểm tra:

`BSIZE % sizeof(struct dinode) == 0`

Điều này yêu cầu kích thước khối chia hết cho kích thước dinode
trong thiết kế hiện tại.

## 6. Những phát hiện ban đầu

- dinode hiện tại chưa có các trường uid, gid và mode.
- inode trong bộ nhớ cần giữ những metadata mà kernel cần sử dụng.
- mkfs và kernel phải thống nhất cách biểu diễn inode trên đĩa.
- Việc thay đổi kích thước dinode có thể ảnh hưởng đến IPB,
  cách bố trí inode và quá trình tạo ảnh hệ thống tệp.
