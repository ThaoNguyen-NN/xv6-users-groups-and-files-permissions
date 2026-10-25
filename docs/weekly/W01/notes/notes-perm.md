# Danh sach cac ham can kiem tra quyen 
##. Các hàm cần kiểm tra quyền

#1. `sys_open()` — `kernel/sysfile.c`

**Chức năng:** Mở tệp để đọc, ghi hoặc tạo tệp mới.

**Yêu cầu về quyền:**
- `O_RDONLY`: yêu cầu quyền đọc.
- `O_WRONLY`: yêu cầu quyền ghi.
- `O_RDWR`: yêu cầu cả quyền đọc và ghi.
- `O_TRUNC`: yêu cầu quyền ghi trước khi cắt ngắn nội dung tệp thông thường.
- `O_CREATE`: cần xem xét quyền ghi và quyền tìm kiếm trên thư mục cha.

**Lưu ý:** Trong xv6, `O_RDONLY` thường có giá trị bằng `0`. Vì vậy, không thể kiểm tra quyền đọc bằng cách chỉ dùng biểu thức `omode & O_RDONLY`.

#2. `sys_chdir()` — `kernel/sysfile.c`

**Chức năng:** Thay đổi thư mục làm việc hiện tại.

**Yêu cầu về quyền:**
- Đường dẫn đích phải trỏ đến một thư mục.
- Tiến trình cần quyền EXECUTE/SEARCH trên thư mục đích để đi vào và truy cập thư mục.
- Quá trình duyệt đường dẫn cũng cần kiểm tra quyền SEARCH trên các thư mục trung gian.

#3. `sys_unlink()` — `kernel/sysfile.c`

**Chức năng:** Xóa một mục trong thư mục, thường là một tệp.

**Yêu cầu về quyền:**
- Kiểm tra quyền WRITE trên thư mục cha.
- Kiểm tra quyền EXECUTE/SEARCH trên thư mục cha.
- Không chỉ dựa vào quyền của chính tệp bị xóa.

**Lưu ý:** Có thể cần bổ sung quy tắc khác, chẳng hạn hạn chế xóa trong thư mục có sticky bit, nếu thiết kế hệ thống hỗ trợ.

#4. `kexec()` — `kernel/exec.c`

**Chức năng:** Nạp chương trình thực thi vào bộ nhớ.

**Yêu cầu về quyền:**
- Kiểm tra quyền EXECUTE của tệp chương trình trước khi nạp.
- Nếu không đủ quyền, trả về lỗi và không nạp chương trình.

#5. `namex()` / `namei()` — `kernel/fs.c`

**Chức năng:** Phân giải đường dẫn thành inode.

**Yêu cầu về quyền:**
- Kiểm tra quyền EXECUTE/SEARCH trên mỗi thư mục được duyệt qua.
- Kiểm tra trước khi tìm thành phần đường dẫn tiếp theo.
- Xem xét cả đường dẫn tuyệt đối và đường dẫn tương đối với thư mục làm việc hiện tại.

**Lưu ý:** `namei()` gọi `namex()`, còn `namex()` sử dụng `dirlookup()` để tìm từng thành phần đường dẫn. Cần lựa chọn vị trí kiểm tra quyền cẩn thận để tránh ảnh hưởng đến các thao tác nội bộ của nhân.

#6. `dirlookup()` — `kernel/fs.c`

**Chức năng:** Tìm một mục có tên tương ứng trong thư mục.

**Yêu cầu về quyền:**
- Kiểm tra các hàm gọi `dirlookup()` để xác định vị trí cần thực hiện kiểm tra quyền SEARCH.
- Không nên thêm kiểm tra quyền tiến trình một cách vô điều kiện tại đây khi chưa xem xét các nơi gọi nội bộ của nhân.


