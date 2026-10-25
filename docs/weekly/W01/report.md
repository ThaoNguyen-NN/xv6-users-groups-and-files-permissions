
## Đóng góp: Khảo sát inode và đề xuất phân quyền

### Công việc đã thực hiện

- Khảo sát struct dinode trong kernel/fs.h và xác định các trường
  metadata hiện có.
- Ghi lại BSIZE = 1024 byte, công thức IPB và chức năng IBLOCK.
- Tìm hiểu struct inode trong kernel/file.h, phân biệt inode trong
  bộ nhớ với dinode lưu trên đĩa.
- Khảo sát mkfs/mkfs.c, đặc biệt là các hàm ialloc() và winode().
- Soạn đề xuất bổ sung mode, uid và gid vào inode.

### Phát hiện

Thêm ba trường uint có thể làm kích thước dinode tăng từ 64 lên
76 byte. Điều này khiến điều kiện BSIZE chia hết cho sizeof(dinode)
trong mkfs không còn thỏa mãn. Cần thống nhất cách bố trí inode
trước khi triển khai.

### Công việc tiếp theo

- Đối chiếu chi tiết các hàm ialloc(), ilock(), iupdate() và stati()
  trong kernel/fs.c.
- Thảo luận thiết kế với nhóm.
- Xác định cách khởi tạo metadata và kiểm thử quyền truy cập.
