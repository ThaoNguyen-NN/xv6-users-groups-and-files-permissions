# Kịch bản demo hệ thống phân quyền trong xv6

**Mục tiêu:** Kiểm tra cơ chế phân quyền dựa trên UID, GID và MODE.

**Trạng thái:** Kịch bản kiểm thử dự kiến, chưa phải kết quả đã chạy thực tế.

## 1. Môi trường kiểm thử

Sử dụng bốn tài khoản:

| Tài khoản | UID | GID | Vai trò |
|---|---:|---:|---|
| root | 0 | 0 | Root |
| alice | 1001 | 100 | Owner |
| bob | 1002 | 100 | Group |
| carol | 1003 | 200 | Other |

Tệp kiểm thử ban đầu:

- Tên: `testfile`
- Chủ sở hữu: Alice
- UID: 1001
- GID: 100

Cần chuẩn bị một chương trình xv6 hợp lệ để kiểm tra quyền thực thi. Không sử dụng tệp văn bản thông thường để kết luận về quyền thực thi.

Để tránh thao tác ghi làm hỏng chương trình, nên sử dụng một tệp dữ liệu riêng cho READ/WRITE và một tệp chương trình riêng cho EXEC, với cùng UID, GID và MODE tương ứng.

## 2. Quy trình chung

Đối với mỗi mức MODE:

1. Khôi phục hoặc tạo lại tệp kiểm thử.
2. Đặt UID = 1001 và GID = 100.
3. Thiết lập MODE cần kiểm tra.
4. Thực hiện các thao tác READ, WRITE, EXEC theo thứ tự Owner, Group, Other, Root.
5. Thực hiện các thao tác CHMOD và CHOWN ở cuối nhóm.
6. Ghi nhận kết quả thực tế.
7. Chuẩn bị trạng thái mới trước khi sang MODE tiếp theo.

Cú pháp lệnh cụ thể phụ thuộc vào các chương trình người dùng mà nhóm đã triển khai.

Các lệnh minh họa:

- `login`: Đăng nhập.
- `su`: Chuyển tài khoản theo cơ chế được triển khai.
- `whoami`: Kiểm tra tài khoản hiện tại.
- `chmod`: Thay đổi quyền.
- `chown`: Thay đổi chủ sở hữu.
- `cat`: Kiểm tra đọc tệp.
- Chạy chương trình kiểm thử: Kiểm tra thực thi.

## 3. Kiểm thử MODE 0600

### Mục tiêu

Chứng minh chỉ Owner được đọc và ghi. Group và Other không được truy cập. Root được đọc và ghi nhưng không được thực thi theo quy tắc riêng của nhóm.

### Thiết lập

Alice tạo tệp và đặt:

`mode = 0600`

`uid = 1001`

`gid = 100`

### Các trường hợp kiểm thử

| Đối tượng | Thao tác | Kết quả mong đợi | Kết quả thực tế |
|---|---|---|---|
| Owner | READ | PASS | |
| Owner | WRITE | PASS | |
| Owner | EXEC | DENIED | |
| Group | READ / WRITE / EXEC | DENIED | |
| Other | READ / WRITE / EXEC | DENIED | |
| Root | READ / WRITE | PASS | |
| Root | EXEC | DENIED | |
| Group | CHMOD | DENIED | |
| Other | CHMOD | DENIED | |
| Owner | CHMOD | PASS | |
| Root | CHMOD | PASS | |

### Kết luận dự kiến

MODE 0600 chỉ cấp quyền đọc và ghi cho Owner. Root có thể đọc và ghi nhưng không thể thực thi vì cả ba nhóm quyền đều không có bit `x`.

## 4. Kiểm thử MODE 0640

### Mục tiêu

Chứng minh hệ thống phân biệt được Group và Other thông qua GID.

### Thiết lập

Khởi tạo lại tệp với:

`mode = 0640`

`uid = 1001`

`gid = 100`

### Các trường hợp kiểm thử

| Đối tượng | Thao tác | Kết quả mong đợi | Kết quả thực tế |
|---|---|---|---|
| Group | READ | PASS | |
| Group | WRITE / EXEC | DENIED | |
| Other | READ / WRITE / EXEC | DENIED | |

### Kết luận dự kiến

Bob có GID bằng 100, trùng với GID của tệp nên được phép đọc.

Carol có GID bằng 200, khác GID của tệp nên không được phép đọc, ghi hoặc thực thi.

Đây là trường hợp quan trọng để chứng minh GID hoạt động chính xác.

## 5. Kiểm thử MODE 0644

### Mục tiêu

Kiểm tra quyền đọc của Group và Other, đồng thời kiểm tra quyền thay đổi MODE và chủ sở hữu.

### Thiết lập

Khởi tạo lại tệp với:

`mode = 0644`

`uid = 1001`

`gid = 100`

### Kiểm tra READ, WRITE, EXEC

| Đối tượng | Thao tác | Kết quả mong đợi | Kết quả thực tế |
|---|---|---|---|
| Owner | READ / WRITE | PASS | |
| Owner | EXEC | DENIED | |
| Group | READ | PASS | |
| Group | WRITE / EXEC | DENIED | |
| Other | READ | PASS | |
| Other | WRITE / EXEC | DENIED | |

### Kiểm tra CHMOD và CHOWN

| Đối tượng | Thao tác | Kết quả mong đợi | Kết quả thực tế |
|---|---|---|---|
| Group | CHMOD | DENIED | |
| Other | CHMOD | DENIED | |
| Group / Other | CHOWN | DENIED | |
| Owner không phải Root | CHOWN | DENIED | |
| Owner | CHMOD | PASS | |
| Root | CHOWN | PASS | |

### Kết luận dự kiến

MODE 0644 cho phép tất cả người dùng đọc tệp nhưng chỉ Owner được ghi.

Hệ thống cũng phải bảo đảm chỉ Owner hoặc Root được đổi MODE và chỉ Root được thay đổi chủ sở hữu.

## 6. Kiểm thử MODE 0755

### Mục tiêu

Chứng minh các nhóm quyền có thể thực thi chương trình và kiểm tra rằng Group, Other không được ghi.

### Thiết lập

Khởi tạo lại tệp chương trình hợp lệ với:

`mode = 0755`

`uid = 1001`

`gid = 100`

### Kiểm tra READ, WRITE, EXEC

| Đối tượng | Thao tác | Kết quả mong đợi | Kết quả thực tế |
|---|---|---|---|
| Owner | READ / WRITE / EXEC | PASS | |
| Group | READ / EXEC | PASS | |
| Group | WRITE | DENIED | |
| Other | READ / EXEC | PASS | |
| Other | WRITE | DENIED | |
| Root | READ / WRITE / EXEC | PASS | |

### Kiểm tra CHMOD

| Đối tượng | Thao tác | Kết quả mong đợi | Kết quả thực tế |
|---|---|---|---|
| Group | CHMOD | DENIED | |
| Other | CHMOD | DENIED | |
| Owner | CHMOD | PASS | |

### Kết luận dự kiến

MODE 0755 cho phép Owner có đầy đủ quyền. Group và Other được đọc, thực thi nhưng không được ghi.

Root được đọc, ghi và thực thi theo chính sách của nhóm.

## 7. Quy tắc kiểm tra quyền

Kernel xác định đối tượng truy cập theo thứ tự:

1. UID bằng 0: Root.
2. UID trùng UID của tệp: Owner.
3. GID trùng GID của tệp: Group.
4. Các trường hợp còn lại: Other.

Sau khi xác định đối tượng, kernel kiểm tra các bit quyền tương ứng.

### Quy tắc đặc biệt của Root

- READ: Bỏ qua kiểm tra bit đọc.
- WRITE: Bỏ qua kiểm tra bit ghi.
- EXEC: Chỉ được phép khi ít nhất một trong ba bit thực thi được bật.

Có thể biểu diễn điều kiện kiểm tra bit thực thi của Root:

`(mode & 0111) != 0`

### Quyền CHMOD và CHOWN

- CHMOD: Chỉ Owner hoặc Root được phép.
- CHOWN: Chỉ Root được phép.

## 8. Lưu ý khi chạy kiểm thử

- Mỗi mức MODE phải bắt đầu từ trạng thái tệp được khôi phục.
- Không để thao tác CHMOD hoặc CHOWN của lần trước ảnh hưởng lần sau.
- Khi kiểm tra EXEC, phải sử dụng chương trình xv6 có định dạng thực thi hợp lệ.
- Sau khi chạy mỗi thao tác, ghi lại kết quả thực tế.
- Không đánh dấu PASS hoặc DENIED chỉ dựa trên kết quả mong đợi.
- Cần kiểm tra thêm việc lưu và đọc lại UID, GID, MODE để bảo đảm metadata không bị mất.

## 9. Tổng kết

Bốn mức quyền 0600, 0640, 0644 và 0755 được sử dụng để kiểm tra các tổ hợp Owner, Group, Other và Root.

Các trường hợp kiểm thử tập trung chứng minh:

- UID xác định chủ sở hữu.
- GID phân biệt người dùng cùng nhóm và khác nhóm.
- MODE quyết định quyền đọc, ghi và thực thi.
- CHMOD chỉ cho Owner hoặc Root.
- CHOWN chỉ cho Root.
- Root có quy tắc đặc biệt đối với thao tác thực thi.

Kết quả thực tế sẽ được ghi nhận sau khi chạy trên phiên bản xv6 đã triển khai hệ thống phân quyền.
