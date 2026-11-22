### Biên dịch và khởi động xv6

- Mở Terminal tại thư mục gốc của dự án.
- Chạy `make qemu` để biên dịch và khởi động xv6.
- Khi hệ thống khởi động thành công, màn hình hiển thị `login:`.
- Nhấn `Ctrl+A`, sau đó nhấn `X` để thoát QEMU.

### Tài khoản mẫu trong `/etc/passwd`

| Tài khoản | Mật khẩu | UID | GID |
|---|---|---|---|
| root | root123 | 0 | 0 |
| alice | alice123 | 1000 | 100 |
| carol | carol123 | 1001 | 100 |
| bob | bob123 | 1002 | 200 |

### Sử dụng lệnh `login`

- Nhập tên tài khoản và mật khẩu tại màn hình `login:`.
- Đăng nhập đúng sẽ chuyển vào shell của người dùng tương ứng.
- Sai mật khẩu hoặc tài khoản không tồn tại sẽ hiển thị `Login incorrect` và yêu cầu nhập lại.
- Sau khi đăng nhập, sử dụng `whoami` để xem tên người dùng hiện tại.
- Sử dụng `id` để kiểm tra UID và GID của tiến trình.
- Nhập `exit` để đăng xuất và quay lại màn hình `login:`.
### Sử dụng lệnh `su`

- `su`: chuyển sang tài khoản root (mặc định).
- `su bob`: chuyển sang tài khoản có tên `bob`.
- Người dùng thông thường phải nhập đúng mật khẩu của tài khoản đích.
- Root có thể chuyển sang tài khoản khác mà không cần nhập mật khẩu.
- Nếu nhập sai mật khẩu, hệ thống từ chối chuyển tài khoản.
- Sau khi chuyển thành công, shell mới chạy với UID và GID của tài khoản đích.
- Sử dụng `whoami` để kiểm tra UID và GID hiện tại.
- Nhấn `Ctrl+D` để thoát shell hiện tại và quay lại shell trước đó.

### chmod
chmod <mode> <file>...
Đổi quyền truy cập của file. <mode> là số bát phân (ví dụ 644, 755; tối đa 4777).
Chỉ root hoặc chủ file mới được đổi; người khác nhận "chmod: cannot change <file>".
Ví dụ: chmod 600 f1   rồi   ls f1   ->   rw------- uid=1000 gid=100

### chown
chown <user> <file>...      chown <uid:gid> <file>...
Đổi chủ và nhóm của file. Dạng <user> lấy uid và gid chính từ /etc/passwd.
Chỉ root được phép; user thường nhận "chown: cannot change <file>".
Ví dụ: chown bob f1   rồi   ls f1   ->   uid=1002 gid=200
