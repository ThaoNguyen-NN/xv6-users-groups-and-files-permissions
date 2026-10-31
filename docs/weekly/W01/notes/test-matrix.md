# UGP-7: Test Matrix Kế hoạch (Permission System)

Bảng kiểm thử các tổ hợp quyền truy cập dựa trên 3 mode cơ bản (600, 644, 755), các đối tượng (Owner, Group, Other) và các thao tác (Read, Write, Exec, Chmod).

| Mode | User Role (Đối tượng) | Operation (Thao tác) | Expected Result (Kỳ vọng) | Actual Result (Thực tế) |
|---|---|---|---|---|
| **600** | Owner | READ | PASS | |
| 600 | Owner | WRITE | PASS | |
| 600 | Owner | EXEC | DENIED | |
| 600 | Owner | CHMOD | PASS | |
| 600 | Group | READ / WRITE / EXEC | DENIED | |
| 600 | Group | CHMOD | DENIED | |
| 600 | Other | READ / WRITE / EXEC | DENIED | |
| 600 | Other | CHMOD | DENIED | |
| **644** | Owner | READ / WRITE | PASS | |
| 644 | Owner | EXEC | DENIED | |
| 644 | Owner | CHMOD | PASS | |
| 644 | Group | READ | PASS | |
| 644 | Group | WRITE / EXEC | DENIED | |
| 644 | Group | CHMOD | DENIED | |
| 644 | Other | READ | PASS | |
| 644 | Other | WRITE / EXEC | DENIED | |
| 644 | Other | CHMOD | DENIED | |
| **755** | Owner | READ / WRITE / EXEC | PASS | |
| 755 | Owner | CHMOD | PASS | |
| 755 | Group | READ / EXEC | PASS | |
| 755 | Group | WRITE | DENIED | |
| 755 | Group | CHMOD | DENIED | |
| 755 | Other | READ / EXEC | PASS | |
| 755 | Other | WRITE | DENIED | |
| 755 | Other | CHMOD | DENIED | |
