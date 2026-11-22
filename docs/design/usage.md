### Sử dụng lệnh `su`

- `su`: chuyển sang tài khoản root (mặc định).
- `su bob`: chuyển sang tài khoản có tên `bob`.
- Người dùng thông thường phải nhập đúng mật khẩu của tài khoản đích.
- Root có thể chuyển sang tài khoản khác mà không cần nhập mật khẩu.
- Nếu nhập sai mật khẩu, hệ thống từ chối chuyển tài khoản.
- Sau khi chuyển thành công, shell mới chạy với UID và GID của tài khoản đích.
- Sử dụng `whoami` để kiểm tra UID và GID hiện tại.
- Nhấn `Ctrl+D` để thoát shell hiện tại và quay lại shell trước đó.
