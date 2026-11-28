# UGP-41 – Thiết kế mật khẩu băm

## 1. Mục tiêu

Trong phiên bản hiện tại, thông tin tài khoản được lưu trong `/etc/passwd` với mật khẩu dạng văn bản thuần. Người dùng có quyền đọc file này có thể biết mật khẩu của các tài khoản khác.

UGP-41 hướng đến các mục tiêu:

- Thay thế mật khẩu dạng văn bản thuần bằng mật khẩu băm có muối (salt).
- Sử dụng thuật toán SHA-256 tự triển khai bằng ngôn ngữ C.
- Sử dụng chung mã nguồn thuật toán băm cho máy host và user space của xv6.
- Sử dụng chung cơ chế xác thực cho `login` và `su`.
- Tự động sinh dữ liệu mật khẩu băm trong quá trình build filesystem.
- Đảm bảo tương thích với các chức năng quản lý người dùng, nhóm và phân quyền đã triển khai.

## 2. Hiện trạng hệ thống

### 2.1. Dữ liệu tài khoản

Theo thiết kế UGP-18 trong `docs/design/api.md` và mục 2.3 của `docs/design/design-proposal.md`, file `/etc/passwd` có định dạng:

name:password:uid:gid

Các tài khoản mẫu hiện tại:

| User  | Mật khẩu |  UID | GID |
| ----- | -------- | ---: | --: |
| root  | root123  |    0 |   0 |
| alice | alice123 | 1000 | 100 |
| carol | carol123 | 1001 | 100 |
| bob   | bob123   | 1002 | 200 |

Alice và Carol thuộc cùng nhóm có GID 100. Bob thuộc nhóm có GID 200.

Các thông tin tài khoản và mật khẩu đăng nhập mẫu được giữ nguyên để tương thích với các bài kiểm thử của những tuần trước.

### 2.2. Cơ chế xác thực hiện tại

Thư viện `user/pwd.c` cung cấp các hàm:

- `getpwnam()`: tra cứu tài khoản theo tên.
- `getpwuid()`: tra cứu tài khoản theo UID.
- `getpwgid()`: tra cứu tài khoản theo GID.
- `pwd_authenticate()`: xác thực mật khẩu.
- `pwd_set_identity()`: chuyển định danh người dùng.

Hàm `pwd_authenticate()` hiện sử dụng `strcmp()` để so sánh trực tiếp mật khẩu nhập vào với mật khẩu lưu trong `/etc/passwd`.

Hai chương trình `login` và `su` sử dụng chung hàm xác thực này.

### 2.3. Hạn chế

- Mật khẩu được lưu trực tiếp trong filesystem.
- User thường có thể đọc mật khẩu của tài khoản khác thông qua `/etc/passwd`.
- Các tài khoản sử dụng cùng mật khẩu sẽ có trường mật khẩu giống nhau.
- Mật khẩu chưa được bảo vệ bằng cơ chế băm.

## 3. Phạm vi và tính kế thừa

UGP-41 là phần mở rộng của cơ chế quản lý tài khoản và xác thực đã triển khai trong các tuần trước.

| Nhiệm vụ | Nội dung kế thừa                                              |
| -------- | ------------------------------------------------------------- |
| UGP-12   | Giữ nguyên metadata mode, UID, GID và cấu trúc inode 128 byte |
| UGP-17   | Giữ nguyên cơ chế quản lý và chuyển đổi UID/GID               |
| UGP-18   | Giữ nguyên cấu trúc passwd và API tra cứu tài khoản           |
| UGP-19   | Kế thừa cơ chế tạo `/etc/passwd` bằng mkfs                    |
| UGP-20   | Giữ nguyên mô hình DAC Owner – Group – Others                 |
| UGP-23   | Giữ nguyên cơ chế kiểm tra quyền truy cập filesystem          |
| UGP-24   | Giữ nguyên cơ chế kiểm tra quyền thực thi và tìm kiếm thư mục |
| UGP-25   | Giữ nguyên cơ chế kiểm tra quyền tạo file                     |
| Tuần 5   | Kế thừa login, su, whoami, id, chmod, chown và setuid         |
| UGP-35   | Giữ nguyên cơ chế lưu trữ dữ liệu khi khởi động lại           |

UGP-41 không yêu cầu bổ sung syscall mới, thay đổi cấu trúc inode hoặc sửa cơ chế phân quyền trong kernel.

Phần thay đổi tập trung vào user space, dữ liệu tài khoản và quá trình build filesystem.

## 4. Thiết kế thuật toán băm

### 4.1. Lựa chọn thuật toán

Sử dụng SHA-256 tự triển khai bằng ngôn ngữ C.

Lý do lựa chọn:

- Phù hợp với phạm vi bài tập và yêu cầu UGP-41.
- Có thể triển khai mà không cần thư viện mã hóa bên ngoài.
- Có thể biên dịch cùng một mã nguồn trên máy host và xv6.
- Không yêu cầu bổ sung chức năng mã hóa trong kernel.
- Có bộ dữ liệu kiểm thử chuẩn để xác minh tính chính xác.

Mã nguồn thuật toán dự kiến được đặt tại:

- `user/sha256.c`
- `user/sha256.h`

File `sha256.c` được sử dụng chung cho cả công cụ sinh mật khẩu trên host và chương trình xác thực trong xv6.

Mã nguồn không phụ thuộc trực tiếp vào syscall của xv6 hoặc thư viện đặc thù của máy host.

### 4.2. Thiết kế muối (salt)

Mỗi tài khoản sử dụng một giá trị muối riêng.

Thông số đề xuất:

| Thành phần      | Giá trị                  |
| --------------- | ------------------------ |
| Độ dài salt     | 16 byte                  |
| Độ dài SHA-256  | 32 byte                  |
| Nguồn sinh salt | `/dev/urandom` trên host |
| Mã hóa salt     | Hexadecimal chữ thường   |
| Mã hóa hash     | Hexadecimal chữ thường   |

Salt được sinh ngẫu nhiên trên máy host trong quá trình tạo dữ liệu tài khoản.

Không cần bổ sung syscall sinh số ngẫu nhiên trong xv6.

Việc sử dụng salt riêng giúp các tài khoản có cùng mật khẩu tạo ra các giá trị băm khác nhau.

Salt không cần được giữ bí mật và được lưu cùng giá trị băm.

### 4.3. Công thức tính giá trị băm

Giá trị băm được tính theo công thức:

```
hash = SHA256(salt_bytes || password_bytes)
```

Trong đó:

- `salt_bytes`: 16 byte muối ban đầu.
- `password_bytes`: các byte của mật khẩu, không bao gồm ký tự kết thúc chuỗi.
- `||`: phép nối hai chuỗi byte.

Công cụ trên host và hàm xác thực trong xv6 phải sử dụng cùng công thức và thứ tự dữ liệu.

## 5. Thiết kế định dạng lưu trữ

### 5.1. Định dạng mới

Giữ nguyên cấu trúc bốn trường của `/etc/passwd` theo UGP-18:

```
name:password:uid:gid
```

Thay nội dung trường `password` bằng:

```
salt$hash
```

Định dạng hoàn chỉnh:

```
name:salt$hash:uid:gid
```

Trong đó:

| Trường | Ý nghĩa                              |
| ------ | ------------------------------------ |
| name   | Tên tài khoản                        |
| salt   | Muối gồm 32 ký tự hexadecimal        |
| hash   | Giá trị băm gồm 64 ký tự hexadecimal |
| uid    | Định danh người dùng                 |
| gid    | Định danh nhóm                       |

Ví dụ minh họa:

```
alice:<salt_hex>$<hash_hex>:1000:100
```

Đây là ví dụ định dạng, không phải giá trị băm thực tế.

### 5.2. Kích thước trường mật khẩu

- Salt: 32 ký tự.
- Dấu phân cách `$`: 1 ký tự.
- Hash: 64 ký tự.
- Tổng cộng: 97 ký tự, không tính ký tự kết thúc chuỗi.

Trường mật khẩu không chứa dấu `:` nên không làm thay đổi số lượng trường của mỗi dòng.

Với `PWD_PASS_MAX = 256` và `LINE_MAX = 512` trong phiên bản hiện tại, định dạng mới phù hợp với kích thước bộ đệm.

Giữ nguyên cấu trúc `struct passwd` và các API tra cứu tài khoản.

Cần kiểm thử lại khả năng xử lý dòng dài bị chia giữa nhiều lần `read()`.

## 6. Thiết kế cơ chế xác thực

### 6.1. Hàm xác thực dùng chung

Giữ nguyên khai báo:

```
int pwd_authenticate(const struct passwd *user,
                     const char *password);
```

Thay đổi phần xử lý bên trong hàm tại `user/pwd.c`.

Quy trình xác thực:

1. Kiểm tra con trỏ tài khoản và mật khẩu đầu vào.
2. Kiểm tra trường mật khẩu đã lưu có đúng định dạng `salt$hash`.
3. Kiểm tra độ dài salt và hash.
4. Kiểm tra các ký tự hexadecimal hợp lệ.
5. Giải mã salt thành 16 byte.
6. Giải mã hash đã lưu thành 32 byte.
7. Tính SHA-256 từ salt và mật khẩu người dùng nhập.
8. So sánh đầy đủ 32 byte của hai giá trị băm, không dừng ngay tại byte khác nhau đầu tiên.
9. Trả về kết quả xác thực.

Quy ước giá trị trả về:

| Giá trị | Ý nghĩa             |
| ------- | ------------------- |
| 0       | Xác thực thành công |
| -1      | Xác thực thất bại   |

Các trường hợp mật khẩu sai, tài khoản không tồn tại, dữ liệu sai định dạng hoặc trường mật khẩu còn ở dạng văn bản thuần phải bị từ chối trong luồng xác thực bằng mật khẩu.

Không hỗ trợ đồng thời hai cơ chế xác thực mật khẩu thường và mật khẩu băm.

### 6.2. Tích hợp với login

Giữ nguyên luồng xử lý hiện tại của `login`:

1. Nhập tên tài khoản.
2. Tra cứu tài khoản bằng `getpwnam()`.
3. Nhập mật khẩu.
4. Gọi `pwd_authenticate()`.
5. Nếu xác thực thành công, chuyển sang định danh của tài khoản và khởi chạy shell.
6. Nếu xác thực thất bại, từ chối đăng nhập.
7. Khi thoát shell, quay lại chương trình login.

Không triển khai thêm một hàm băm riêng trong `login.c`.

### 6.3. Tích hợp với su

Chương trình `su` tiếp tục sử dụng chung hàm `pwd_authenticate()`.

Giữ nguyên:

- Cơ chế chuyển đổi UID/GID.
- Cơ chế real UID và effective UID đã triển khai.
- Cơ chế setuid của chương trình `su`.
- Quy tắc real root được sử dụng `su` mà không cần nhập mật khẩu.
- Quy tắc không để lộ quyền root sau khi thoát shell.

Các ca kiểm thử mật khẩu đúng và sai của `su` phải được thực hiện từ user thường để không đi qua ngoại lệ xác thực của real root.

### 6.4. Tương thích với whoami và id

Không thay đổi thông tin tên tài khoản, UID hoặc GID.

Các chương trình `whoami` và `id` tiếp tục hoạt động theo cơ chế tra cứu và hiển thị định danh hiện tại.

## 7. Thiết kế công cụ sinh mật khẩu trên host

### 7.1. File dữ liệu đầu vào

Chuyển dữ liệu tài khoản mẫu hiện tại sang:

```
etc/passwd.in
```

File này giữ định dạng:

```
name:password:uid:gid
```

Đây là dữ liệu phục vụ build và kiểm thử trên máy host.

Không đưa `etc/passwd.in` vào `fs.img`.

### 7.2. Công cụ sinh mật khẩu

Dự kiến xây dựng công cụ:

```
tools/genpasswd.c
```

Công cụ được biên dịch bằng trình biên dịch C trên host và sử dụng chung mã nguồn `user/sha256.c`.

Quy trình xử lý:

1. Đọc từng dòng từ `etc/passwd.in`.
2. Kiểm tra định dạng dữ liệu tài khoản.
3. Sinh salt ngẫu nhiên cho từng tài khoản.
4. Tính giá trị băm SHA-256.
5. Chuyển salt và hash sang hexadecimal.
6. Ghi dữ liệu theo định dạng mới vào `etc/passwd`.
7. Báo lỗi và dừng quá trình build nếu dữ liệu đầu vào không hợp lệ hoặc không thể sinh salt.

File đầu ra nên được tạo qua file tạm và chỉ thay thế `etc/passwd` sau khi quá trình sinh dữ liệu thành công.

Không in mật khẩu dạng văn bản thuần trong thông báo lỗi.

### 7.3. Tích hợp Makefile

Bổ sung các quy tắc build để:

- Biên dịch công cụ `genpasswd` bằng trình biên dịch của host.
- Biên dịch SHA-256 cho cả host và xv6.
- Sinh `etc/passwd` từ `etc/passwd.in` trước khi tạo `fs.img`.
- Khai báo đầy đủ các file phụ thuộc.
- Cập nhật quy tắc `make clean`.
- Cập nhật `.gitignore` cho file và chương trình được sinh tự động.
- Chuyển `etc/passwd` từ file được quản lý trực tiếp trong Git thành file sinh tự động khi triển khai.

Luồng build dự kiến:

```
etc/passwd.in
      |
      v
genpasswd + sha256.c
      |
      v
etc/passwd
      |
      v
     mkfs
      |
      v
    fs.img
```

Giữ nguyên đường dẫn đầu ra `etc/passwd` vì `mkfs/mkfs.c` hiện có xử lý riêng đối với đường dẫn này.

Không cần thay đổi cấu trúc filesystem hoặc định dạng inode.

## 8. Bảo vệ dữ liệu tài khoản

### 8.1. Quyền truy cập

Theo thiết kế UGP-12 trong `docs/design/api.md`, giữ nguyên metadata:

| Đối tượng     | Mode | UID | GID |
| ------------- | ---: | --: | --: |
| `/etc`        | 0755 |   0 |   0 |
| `/etc/passwd` | 0644 |   0 |   0 |

Quy tắc:

- Root có quyền đọc và ghi dữ liệu tài khoản.
- User thường được đọc `/etc/passwd` để phục vụ tra cứu thông tin.
- User thường không được ghi hoặc thay đổi nội dung `/etc/passwd`.
- User thường không được xóa hoặc thay thế `/etc/passwd` thông qua thư mục `/etc`.

Việc bảo vệ dữ liệu tiếp tục dựa trên mô hình DAC đã được triển khai từ các tuần trước.

Không thay đổi quyền `/etc/passwd` thành `0600` vì các chương trình tra cứu tài khoản của user thường cần đọc file này.

### 8.2. Tính bền vững dữ liệu

Kế thừa kết quả của UGP-35:

- Không sinh lại salt hoặc mật khẩu băm mỗi lần khởi động xv6.
- Không tạo lại `fs.img` khi thực hiện kiểm thử reboot với cùng filesystem.
- File `etc/passwd` chỉ được sinh lại khi cần thiết trong quá trình build.
- Dữ liệu tài khoản và metadata phải được giữ nguyên khi khởi động lại với cùng `fs.img`.

Khi chuyển từ phiên bản mật khẩu thường sang phiên bản mật khẩu băm, cần tạo filesystem mới.

Không hỗ trợ tự động chuyển đổi filesystem cũ trong phạm vi UGP-41.

## 9. Các file dự kiến thay đổi

| File                              | Nội dung                                        |
| --------------------------------- | ----------------------------------------------- |
| `user/sha256.c`                   | Cài đặt SHA-256 dùng chung cho host và xv6      |
| `user/sha256.h`                   | Khai báo API SHA-256                            |
| `user/pwd.c`                      | Thay đổi cơ chế xác thực mật khẩu               |
| `user/pwdtest.c`                  | Cập nhật fixture và bổ sung test xác thực       |
| `tools/genpasswd.c`               | Công cụ sinh mật khẩu băm trên host             |
| `etc/passwd.in`                   | Dữ liệu tài khoản mẫu                           |
| `etc/passwd`                      | File mật khẩu băm được sinh tự động             |
| `Makefile`                        | Tích hợp build công cụ và dữ liệu tài khoản     |
| `.gitignore`                      | Bỏ qua các file được sinh tự động               |
| `docs/design/api.md`              | Cập nhật đặc tả trường mật khẩu và API xác thực |
| `docs/design/usage.md`            | Cập nhật hướng dẫn sử dụng nếu cần              |
| `docs/design/hashed-passwords.md` | Tài liệu thiết kế UGP-41                        |

Các file kiểm thử khác có chứa dữ liệu mật khẩu thường sẽ được rà soát trong quá trình triển khai.

Không dự kiến thay đổi `kernel/`, `login.c` hoặc `su.c` nếu các API hiện tại đáp ứng đầy đủ yêu cầu.

## 10. Kế hoạch kiểm thử

### 10.1. Kiểm thử thuật toán và dữ liệu

| Mã test | Nội dung                                         | Kết quả mong đợi         |
| ------- | ------------------------------------------------ | ------------------------ |
| HP01    | SHA-256 với chuỗi rỗng                           | Khớp giá trị chuẩn       |
| HP02    | SHA-256 với chuỗi `abc`                          | Khớp giá trị chuẩn       |
| HP03    | So sánh kết quả trên host và xv6                 | Hai kết quả giống nhau   |
| HP04    | Cùng mật khẩu, khác salt                         | Giá trị băm khác nhau    |
| HP05    | Kiểm tra định dạng `salt$hash`                   | Đúng độ dài và định dạng |
| HP06    | Salt hoặc hash sai định dạng                     | Xác thực thất bại        |
| HP07    | Trường mật khẩu dạng văn bản thuần               | Xác thực thất bại        |
| HP08    | Dữ liệu đầu vào công cụ sinh passwd không hợp lệ | Build báo lỗi            |

### 10.2. Kiểm thử chức năng

| Mã test | Nội dung                            | Kết quả mong đợi              |
| ------- | ----------------------------------- | ----------------------------- |
| HP09    | Login đúng mật khẩu                 | Thành công                    |
| HP10    | Login sai mật khẩu                  | Bị từ chối                    |
| HP11    | Login user không tồn tại            | Bị từ chối                    |
| HP12    | Thoát shell sau login               | Quay lại login                |
| HP13    | Su từ user thường với mật khẩu đúng | Thành công                    |
| HP14    | Su từ user thường với mật khẩu sai  | Bị từ chối                    |
| HP15    | Su từ real root                     | Giữ đúng quy tắc hiện tại     |
| HP16    | Thoát shell sau su                  | Quay về định danh trước đó    |
| HP17    | Chạy whoami và id                   | Thông tin định danh chính xác |

### 10.3. Kiểm thử thư viện passwd

Giữ nguyên các ca kiểm thử của UGP-18:

- Tra cứu root, alice, carol và bob.
- Tra cứu tài khoản theo tên.
- Tra cứu tài khoản theo UID/GID.
- Tra cứu tài khoản không tồn tại.
- Xử lý dữ liệu sai định dạng.
- Xử lý dòng dài bị chia giữa nhiều lần `read()`.

Bổ sung các ca kiểm thử xác thực mật khẩu đúng, sai và dữ liệu băm không hợp lệ.

Cập nhật fixture trong `pwdtest` để tương thích với định dạng mới.

Tuân theo quy ước in kết quả kiểm thử của nhóm:

```
PASS: <noi dung>
FAIL: <noi dung>
```

Dòng tổng kết:

```
pwdtest: ALL PASSED
```

Hoặc:

```
pwdtest: FAILED
```

### 10.4. Kiểm thử filesystem và phân quyền

| Mã test | Nội dung                                    | Kết quả mong đợi                                     |
| ------- | ------------------------------------------- | ---------------------------------------------------- |
| HP18    | Kiểm tra `/etc/passwd`                      | Chỉ lưu salt và hash                                 |
| HP19    | Kiểm tra metadata `/etc/passwd`             | Mode 0644, UID/GID 0/0                               |
| HP20    | User thường đọc `/etc/passwd`               | Thành công                                           |
| HP21    | User thường ghi `/etc/passwd`               | Bị từ chối                                           |
| HP22    | User thường xóa hoặc thay thế `/etc/passwd` | Bị từ chối                                           |
| HP23    | Reboot với cùng `fs.img`                    | Salt, hash và metadata không đổi                     |
| HP24    | Kiểm tra mật khẩu thường trong `fs.img`     | Không còn mật khẩu thường theo phạm vi đã thống nhất |

Cần rà soát cả các chương trình kiểm thử được đóng gói trong `fs.img`, tránh để dữ liệu mật khẩu thường tồn tại trong các binary kiểm thử.

Các mật khẩu mẫu phục vụ kiểm thử trên host có thể tiếp tục được lưu trong `etc/passwd.in` nhưng không được đóng gói trực tiếp vào filesystem.

### 10.5. Kiểm thử hồi quy

- Chạy `make fmt` trước khi mở Pull Request.
- Kiểm tra quá trình build kernel và filesystem.
- Chạy `pwdtest`.
- Kiểm tra `login` và `su` trên QEMU.
- Chạy `usertests` với quyền root.
- Chạy `grind` khoảng 5 phút.
- Kiểm tra CI.

Tiêu chí hồi quy:

- `usertests`: ALL TESTS PASSED.
- `grind`: không xảy ra panic trong thời gian kiểm thử.
- CI chạy thành công.

Các kết quả thực tế được lưu tại:

```
docs/weekly/W06/evidence/
```

Các ca chưa chạy không được đánh dấu PASS.

## 11. Hạn chế và rủi ro

- SHA-256 có muối được sử dụng trong phạm vi bài tập hệ điều hành, không phải thuật toán lưu trữ mật khẩu chuyên dụng cho hệ thống thực tế.
- SHA-256 có tốc độ tính toán cao nên vẫn có nguy cơ bị dò mật khẩu ngoại tuyến.
- Các hệ thống thực tế nên sử dụng thuật toán chuyên dụng như Argon2id.
- Mật khẩu mẫu vẫn tồn tại trong dữ liệu kiểm thử trên host và lịch sử Git.
- Mật khẩu nhập trên console xv6 vẫn hiển thị trên màn hình.
- Không triển khai chức năng đổi mật khẩu hoặc quản lý tài khoản mới trong UGP-41.
- Không hỗ trợ chuyển đổi tự động filesystem cũ.
- Việc bảo vệ `/etc/passwd` phụ thuộc vào tính chính xác của cơ chế phân quyền hiện tại.

Các hạn chế còn tồn tại sẽ được trao đổi với Nguyên để cập nhật danh sách known issues của nhóm.

## 12. Tiêu chí hoàn thành

UGP-41 được xem là hoàn thành khi:

- Tài liệu thiết kế được nhóm thống nhất.
- SHA-256 được tự triển khai bằng C và sử dụng chung cho host và xv6.
- Mỗi tài khoản có salt riêng.
- Trường mật khẩu có định dạng `salt$hash`.
- Công cụ sinh `etc/passwd` được tích hợp vào quá trình build.
- Login và su sử dụng chung hàm `pwd_authenticate()`.
- Mật khẩu đúng được chấp nhận, mật khẩu sai bị từ chối.
- `/etc/passwd` không còn mật khẩu dạng văn bản thuần.
- Kiểm tra `fs.img` đạt yêu cầu đã thống nhất.
- Các API tra cứu tài khoản và chức năng phân quyền hiện tại vẫn hoạt động.
- `pwdtest` và `usertests` chạy thành công.
- `grind` chạy khoảng 5 phút không xảy ra panic.
- Pull Request được Đức Anh review và CI chạy thành công.
- Hoàn thành kiểm thử hồi quy sau merge.
