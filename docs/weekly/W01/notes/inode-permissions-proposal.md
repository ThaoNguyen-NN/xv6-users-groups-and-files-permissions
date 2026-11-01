# Đề xuất bổ sung quyền truy cập và chủ sở hữu inode

## 1. Vấn đề hiện tại

Cấu trúc dinode trong kernel/fs.h lưu loại tệp, thông tin thiết bị,
số liên kết, kích thước và địa chỉ các khối dữ liệu.

Cấu trúc hiện tại chưa có trường lưu người dùng sở hữu, nhóm sở hữu
và quyền truy cập của tệp.

## 2. Các trường đề xuất

Đề xuất bổ sung ba trường:

- `mode`: Lưu quyền đọc, ghi và thực thi.
- `uid`: Mã định danh người dùng sở hữu tệp.
- `gid`: Mã định danh nhóm sở hữu tệp.

Đề xuất ban đầu là dùng kiểu `uint` cho cả ba trường. Nhóm cần
thống nhất lựa chọn này trước khi triển khai.

Trường `type` hiện tại tiếp tục dùng để phân biệt loại đối tượng;
`mode` chỉ biểu diễn quyền truy cập, không thay thế `type`.

## 3. Vị trí cần xem xét

### kernel/fs.h

Bổ sung các trường vào struct dinode để lưu metadata trên đĩa.

### kernel/file.h

Bổ sung các trường tương ứng vào struct inode để kernel sử dụng
metadata khi inode nằm trong bộ nhớ.

### kernel/fs.c

Cập nhật hoặc kiểm tra các hàm:

- `ialloc()`: Khởi tạo metadata cho inode mới.
- `ilock()`: Nạp metadata từ dinode vào inode trong bộ nhớ.
- `iupdate()`: Ghi metadata từ inode xuống đĩa.
- `stati()`: Xem xét cung cấp thông tin mới nếu giao diện stat cần.

### mkfs/mkfs.c

Xem xét việc khởi tạo mode, uid và gid cho các inode ban đầu,
bao gồm thư mục gốc và các tệp được đưa vào ảnh hệ thống tệp.

Hàm ialloc() trong mkfs cần được phân biệt với hàm ialloc()
trong kernel: chúng chạy ở hai môi trường khác nhau.

## 4. Ảnh hưởng đến kích thước dinode

Theo cấu trúc ban đầu của xv6:

- BSIZE = 1024 byte.
- Kích thước dinode = 64 byte.
- IPB = 1024 / 64 = 16 inode mỗi khối.

Nếu thêm ba trường uint, mỗi trường 4 byte, kích thước dinode
dự kiến tăng lên 76 byte.

Khi đó, điều kiện hiện có trong mkfs:

`BSIZE % sizeof(struct dinode) == 0`

không còn thỏa mãn, vì 1024 không chia hết cho 76.

Do đó, nhóm phải quyết định cách bố trí inode mới trên đĩa và
cập nhật cách tính toán, kiểm tra tương ứng trong mkfs và kernel.
Không nên chỉ thêm trường rồi coi như hoàn thành.

## 5. Chính sách cần thống nhất

Nhóm cần thống nhất:

- UID và GID dành cho root.
- UID/GID mặc định cho các inode ban đầu.
- Quyền mặc định cho tệp thường và thư mục.
- Quyền của tệp được tạo khi xv6 đang chạy.
- Cách xử lý ảnh hệ thống tệp được tạo theo bố cục cũ.

Một phương án để thảo luận là dùng UID/GID bằng 0 cho root,
quyền 0644 cho tệp thường và 0755 cho thư mục. Đây là đề xuất
ban đầu, chưa phải chính sách đã được nhóm thông qua.

## 6. Kiểm thử dự kiến

Sau khi triển khai, cần kiểm tra:

1. Metadata được khởi tạo đúng.
2. Metadata được ghi xuống đĩa và đọc lại chính xác.
3. Quyền và chủ sở hữu vẫn đúng sau khi khởi động lại xv6.
4. Người dùng được phép hoặc bị từ chối truy cập đúng theo quyền.
5. Cập nhật một inode không làm hỏng các inode khác trong cùng khối.
6. mkfs tạo được ảnh hệ thống tệp với bố cục mới.
