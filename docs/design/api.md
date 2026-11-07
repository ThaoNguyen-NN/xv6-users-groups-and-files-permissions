## Process identity (UGP-17)

Mỗi process có `uid` và `gid` (`uint`). Process đầu tiên (`init`) là root (uid 0, gid 0).
`fork` sao chép `uid/gid` của cha; `exec` giữ nguyên.

| Syscall | Số | Trả về | Lỗi (-1) |
|---|---|---|---|
| `int getuid(void)` | 23 | uid của process | không |
| `int getgid(void)` | 24 | gid của process | không |
| `int setuid(int uid)` | 25 | 0 | không phải root (uid hiện tại khác 0); `uid < 0` |
| `int setgid(int gid)` | 26 | 0 | không phải root (uid hiện tại khác 0); `gid < 0` |

Quy tắc:
- Chỉ root được gọi `setuid` và `setgid`. Việc đổi một chiều: sau khi hạ xuống user thường thì không đổi lại được.
- Khi hạ quyền phải gọi `setgid` trước, `setuid` sau, vì sau `setuid` xuống user thường thì `setgid` bị từ chối.
- Số 27 và 28 dành cho `chmod` và `chown`.

Chương trình: `whoami` in `uid=<n> gid=<n>` (sẽ in tên user khi có `/etc/passwd`); `uidtest` kiểm tra các quy tắc trên.

## Test dự kiến

Định dạng in: mỗi ca một dòng `PASS: <nội dung>` hoặc `FAIL: <nội dung>`, dòng cuối `<tên test>: ALL PASSED` hoặc `<tên test>: FAILED`.

- [x] Nguyên: `uidtest` kiểm tra `getuid/getgid/setuid/setgid` và việc kế thừa qua `fork/exec` (`user/uidtest.c`).
- [ ] Thư: `pwdtest` kiểm tra tra cứu user/group.
  - Cần: `user/pwd.c` (`getpwnam`, `getpwuid`) và `/etc/passwd` có trong `fs.img`.
  - Đạt khi: `root` cho uid 0 gid 0; `alice` cho 1000/100; `carol` cho 1001/100 (cùng gid với alice); `bob` cho 1002/200; tên không tồn tại và uid không tồn tại (ví dụ 9999) đều báo không tìm thấy; `getpwuid(1000)` trả về `alice`; dòng passwd dài vẫn đọc đúng khi bị cắt giữa hai lần `read`.
- [ ] Đức Anh: `metatest` kiểm tra `mode/uid/gid` của file qua `stat`.
  - Cần: `struct stat` có `mode/uid/gid` và `stati()` sao chép các trường này.
  - Đạt khi (theo thiết kế UGP-12): file do mkfs tạo (`/README`) có mode 0644, uid 0, gid 0, còn chương trình (`/init`) có mode 0755; file mới do root tạo có mode 0644, thư mục mới 0755; `console` 0666; file tạo bởi process uid 1000 gid 100 mang đúng uid/gid đó; các giá trị còn nguyên sau khi khởi động lại qemu mà không dựng lại `fs.img`.

## User lookup API (UGP-18)

Người thực hiện: Trần Thị Thư _ 20233664

### Dữ liệu /etc/passwd

Định dạng mỗi dòng        name:password:uid:gid

Tài khoản mẫu:
- root: UID 0, GID 0
- alice: UID 1000, GID 100
- carol: UID 1001, GID 100
- bob: UID 1002, GID 200

### Các hàm

1. getpwnam(const char *name)
   - Tìm user theo tên.
   - Trả về con trỏ struct passwd nếu tìm thấy.
   - Trả NULL nếu không tìm thấy hoặc không đọc được file.

2. getpwuid(int uid)
   - Tìm user theo UID.
   - Trả về con trỏ struct passwd nếu tìm thấy.
   - Trả NULL nếu không tìm thấy hoặc lỗi.

3. getpwgid(int gid)
   - Tìm user đầu tiên có GID tương ứng.
   - Trả về NULL nếu không tìm thấy hoặc lỗi.

### Xử lý dữ liệu

- Đọc file bằng open(), read(), close().
- Ghép dữ liệu qua nhiều lần read().
- Phân tích các trường name, password, uid, gid.
- Bỏ qua dòng sai định dạng.
- Kiểm tra UID/GID là số nguyên hợp lệ.
- Giới hạn độ dài dòng bằng bộ đệm.

Kết quả tra cứu dùng vùng nhớ static,
có thể bị ghi đè bởi lần tra cứu thành công tiếp theo.

### Kiểm thử

Chương trình: user/pwdtest.c
Lệnh: pwdtest --fixture

Kết quả ngày 07/11/2026:
- 10 tests passed
- 0 tests failed
- pwdtest: ALL PASSED

Ghi chú:
Kiểm thử sử dụng dữ liệu fixture tạm.
Kiểm thử tích hợp với /etc/passwd trong fs.img sẽ thực hiện sau khi hoàn thành UGP-19.
