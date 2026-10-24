# Danh sách các hàm cần kiểm tra quyền (Permission Checks)

Để hệ thống phân quyền Unix-like hoạt động, ta cần chèn logic kiểm tra quyền (so sánh UID/GID của tiến trình với metadata của inode) vào các vị trí sau trong mã nguồn kernel:

1. **`sys_open` (kernel/sysfile.c)**
   - Thao tác: Mở file (để đọc/ghi) hoặc thư mục.
   - Kiểm tra: Quyền READ (nếu mở `O_RDONLY`/`O_RDWR`) và quyền WRITE (nếu mở `O_WRONLY`/`O_RDWR`) trên file đó.

2. **`sys_chdir` (kernel/sysfile.c)**
   - Thao tác: Thay đổi thư mục làm việc (lệnh cd).
   - Kiểm tra: Bắt buộc phải có quyền EXECUTE (đóng vai trò quyền traverse/truy cập) trên thư mục đích.

3. **`sys_unlink` (kernel/sysfile.c)**
   - Thao tác: Xóa file.
   - Kiểm tra: KHÔNG kiểm tra quyền trên file bị xóa. Phải kiểm tra quyền WRITE và EXECUTE trên **thư mục cha** chứa file đó.

4. **`exec` (kernel/exec.c)**
   - Thao tác: Thực thi một chương trình.
   - Kiểm tra: Quyền EXECUTE trên file thực thi. Nếu không có quyền, không nạp chương trình vào bộ nhớ.

5. **`dirlookup` / `namei` (kernel/fs.c)**
   - Thao tác: Phân giải đường dẫn (chuyển path thành inode).
   - Kiểm tra: Tại mỗi cấp thư mục đi qua, tiến trình đều phải có quyền EXECUTE trên thư mục đó
