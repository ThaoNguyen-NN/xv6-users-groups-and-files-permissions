# Test Matrix Kế hoạch (Permission System)

Bảng kiểm thử các tổ hợp quyền truy cập theo mode file (600, 640, 644, 755), các đối tượng (Owner, Group, Other, Root) và các thao tác (Read, Write, Exec, Chmod, Chown).

## Cách dùng

Bảng chia thành 4 nhóm theo mode. Mỗi nhóm chỉ cần tạo file một lần rồi chạy từ trên xuống:

1. Tạo file thuộc Owner với mode của nhóm (và gid của file).
2. Chạy các dòng READ / WRITE / EXEC lần lượt theo Owner → Group → Other → Root.
3. Chạy các dòng CHMOD / CHOWN ở cuối nhóm. Dòng DENIED đặt trước, dòng PASS đặt sau cùng vì làm đổi mode hoặc owner của file. Sau khi chạy xong, tạo lại file cho nhóm kế tiếp.

## Bảng

| Mode | User Role (Đối tượng) | Operation (Thao tác) | Expected Result (Kỳ vọng) | Actual Result (Thực tế) |
|---|---|---|---|---|
| **600** | Owner | READ | PASS | |
| 600 | Owner | WRITE | PASS | |
| 600 | Owner | EXEC | DENIED | |
| 600 | Group | READ / WRITE / EXEC | DENIED | |
| 600 | Other | READ / WRITE / EXEC | DENIED | |
| 600 | Root | READ / WRITE | PASS | |
| 600 | Root | EXEC | DENIED | |
| 600 | Group | CHMOD | DENIED | |
| 600 | Other | CHMOD | DENIED | |
| 600 | Owner | CHMOD | PASS | |
| 600 | Root | CHMOD | PASS | |
| **640** | Group | READ | PASS | |
| 640 | Group | WRITE / EXEC | DENIED | |
| 640 | Other | READ / WRITE / EXEC | DENIED | |
| **644** | Owner | READ / WRITE | PASS | |
| 644 | Owner | EXEC | DENIED | |
| 644 | Group | READ | PASS | |
| 644 | Group | WRITE / EXEC | DENIED | |
| 644 | Other | READ | PASS | |
| 644 | Other | WRITE / EXEC | DENIED | |
| 644 | Group | CHMOD | DENIED | |
| 644 | Other | CHMOD | DENIED | |
| 644 | Group / Other | CHOWN | DENIED | |
| 644 | Owner (không phải root) | CHOWN | DENIED | |
| 644 | Owner | CHMOD | PASS | |
| 644 | Root | CHOWN | PASS | |
| **755** | Owner | READ / WRITE / EXEC | PASS | |
| 755 | Group | READ / EXEC | PASS | |
| 755 | Group | WRITE | DENIED | |
| 755 | Other | READ / EXEC | PASS | |
| 755 | Other | WRITE | DENIED | |
| 755 | Root | READ / WRITE / EXEC | PASS | |
| 755 | Group | CHMOD | DENIED | |
| 755 | Other | CHMOD | DENIED | |
| 755 | Owner | CHMOD | PASS | |

## Quy ước

- **Group** = user không phải owner nhưng có `gid` trùng `gid` của file. User khác gid là Other.
- **Root (uid 0)** bỏ qua kiểm tra READ/WRITE và luôn được CHMOD, CHOWN. EXEC chỉ PASS khi file có ít nhất một bit x.
- **CHOWN:** chỉ root được đổi owner. Owner thường không được đổi, kể cả với file của chính mình.
- **CHMOD:** chỉ owner hoặc root.
- **Mode 640** là mode duy nhất tách được Group khỏi Other, vì ở 600, 644 và 755 hai lớp này cho kết quả giống nhau.

## Giả định

- Mode mặc định theo nhóm: 0644 cho file, 0755 cho thư mục.
- Chưa xét supplementary groups: mỗi process chỉ có một `gid`.
- Cột Actual Result để trống, điền khi chạy test thực tế. Khi viết test cần cẩn thận `O_RDONLY = 0`.
