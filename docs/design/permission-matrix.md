# UGP-38 – Permission Matrix

## 1. Test information

- **Test date:** 2026-11-27
- **Test command:** `python3 test-xv6.py accesstest`
- **Test program:** `user/accesstest.c`
- **Filesystem fixtures:** `/perm000`, `/perm600`, `/perm640`, `/perm644`, `/perm755`, `/perm4755`
- **Fixture owner:** UID 1000, GID 100
- **Total test cases:** 144
- **Passed:** 144
- **Failed:** 0
- **Overall result:** PASS

## 2. Tested identities

| Role | Account | UID | GID | Description |
|---|---|---:|---:|---|
| Owner | alice | 1000 | 100 | File/directory owner |
| Group | carol | 1001 | 100 | Non-owner in the same group |
| Other | bob | 1002 | 200 | Different group |
| Root | root | 0 | 0 | Superuser |

The test uses the process's primary GID. Supplementary groups are not tested.

## 3. Test operations

| Object | Operation | Actual operation used |
|---|---|---|
| File | READ | Open with `O_RDONLY`, then read one byte |
| File | WRITE | Open with `O_WRONLY`, then write one byte |
| File | EXEC | Execute the ELF fixture using `exec()` |
| Directory | SEARCH | Change directory using `chdir()` |
| Directory | CREATE | Create a file using `O_CREATE \| O_WRONLY` |
| Directory | DELETE | Remove an existing file using `unlink()` |

For file WRITE, the test restores the original first ELF byte (`0x7f`) so the fixture remains executable for later cases.

## 4. Permission policy and observations

The matrix covers modes `000`, `600`, `640`, `644`, `755`, and `4755`.

- File access for non-root processes is determined by owner, group, or other permission bits.
- Root can bypass ordinary read/write permission checks.
- Root execution is still denied when the file has no execute bit set.
- Directory SEARCH requires execute permission.
- Directory CREATE and DELETE require write and execute permission on the parent directory.
- Root still requires an execute bit for directory operations that request execute permission.
- Mode `4755` has the same ordinary rwx access result as `0755` in this permission matrix.

**Setuid limitation:** This matrix verifies whether execution is allowed. It does not independently verify the effective UID transition after executing a Setuid file. That behavior must be checked by a separate identity test.

## 5. Results summary

| Test group | Test cases | Passed | Failed | Result |
|---|---:|---:|---:|---|
| File permissions | 72 | 72 | 0 | PASS |
| Directory permissions | 72 | 72 | 0 | PASS |
| **Total** | **144** | **144** | **0** | **PASS** |

## 6. Complete actual results

Every row below is parsed from the captured QEMU output. `Expected` and `Actual` are the values printed by the test program.

| Type | Mode | Role | Operation | Expected | Actual | PASS/FAIL |
|---|---:|---|---|---|---|---|
| File | 000 | Owner | READ | DENY | DENY | **PASS** |
| File | 000 | Owner | WRITE | DENY | DENY | **PASS** |
| File | 000 | Owner | EXEC | DENY | DENY | **PASS** |
| File | 000 | Group | READ | DENY | DENY | **PASS** |
| File | 000 | Group | WRITE | DENY | DENY | **PASS** |
| File | 000 | Group | EXEC | DENY | DENY | **PASS** |
| File | 000 | Other | READ | DENY | DENY | **PASS** |
| File | 000 | Other | WRITE | DENY | DENY | **PASS** |
| File | 000 | Other | EXEC | DENY | DENY | **PASS** |
| File | 000 | Root | READ | ALLOW | ALLOW | **PASS** |
| File | 000 | Root | WRITE | ALLOW | ALLOW | **PASS** |
| File | 000 | Root | EXEC | DENY | DENY | **PASS** |
| File | 600 | Owner | READ | ALLOW | ALLOW | **PASS** |
| File | 600 | Owner | WRITE | ALLOW | ALLOW | **PASS** |
| File | 600 | Owner | EXEC | DENY | DENY | **PASS** |
| File | 600 | Group | READ | DENY | DENY | **PASS** |
| File | 600 | Group | WRITE | DENY | DENY | **PASS** |
| File | 600 | Group | EXEC | DENY | DENY | **PASS** |
| File | 600 | Other | READ | DENY | DENY | **PASS** |
| File | 600 | Other | WRITE | DENY | DENY | **PASS** |
| File | 600 | Other | EXEC | DENY | DENY | **PASS** |
| File | 600 | Root | READ | ALLOW | ALLOW | **PASS** |
| File | 600 | Root | WRITE | ALLOW | ALLOW | **PASS** |
| File | 600 | Root | EXEC | DENY | DENY | **PASS** |
| File | 640 | Owner | READ | ALLOW | ALLOW | **PASS** |
| File | 640 | Owner | WRITE | ALLOW | ALLOW | **PASS** |
| File | 640 | Owner | EXEC | DENY | DENY | **PASS** |
| File | 640 | Group | READ | ALLOW | ALLOW | **PASS** |
| File | 640 | Group | WRITE | DENY | DENY | **PASS** |
| File | 640 | Group | EXEC | DENY | DENY | **PASS** |
| File | 640 | Other | READ | DENY | DENY | **PASS** |
| File | 640 | Other | WRITE | DENY | DENY | **PASS** |
| File | 640 | Other | EXEC | DENY | DENY | **PASS** |
| File | 640 | Root | READ | ALLOW | ALLOW | **PASS** |
| File | 640 | Root | WRITE | ALLOW | ALLOW | **PASS** |
| File | 640 | Root | EXEC | DENY | DENY | **PASS** |
| File | 644 | Owner | READ | ALLOW | ALLOW | **PASS** |
| File | 644 | Owner | WRITE | ALLOW | ALLOW | **PASS** |
| File | 644 | Owner | EXEC | DENY | DENY | **PASS** |
| File | 644 | Group | READ | ALLOW | ALLOW | **PASS** |
| File | 644 | Group | WRITE | DENY | DENY | **PASS** |
| File | 644 | Group | EXEC | DENY | DENY | **PASS** |
| File | 644 | Other | READ | ALLOW | ALLOW | **PASS** |
| File | 644 | Other | WRITE | DENY | DENY | **PASS** |
| File | 644 | Other | EXEC | DENY | DENY | **PASS** |
| File | 644 | Root | READ | ALLOW | ALLOW | **PASS** |
| File | 644 | Root | WRITE | ALLOW | ALLOW | **PASS** |
| File | 644 | Root | EXEC | DENY | DENY | **PASS** |
| File | 755 | Owner | READ | ALLOW | ALLOW | **PASS** |
| File | 755 | Owner | WRITE | ALLOW | ALLOW | **PASS** |
| File | 755 | Owner | EXEC | ALLOW | ALLOW | **PASS** |
| File | 755 | Group | READ | ALLOW | ALLOW | **PASS** |
| File | 755 | Group | WRITE | DENY | DENY | **PASS** |
| File | 755 | Group | EXEC | ALLOW | ALLOW | **PASS** |
| File | 755 | Other | READ | ALLOW | ALLOW | **PASS** |
| File | 755 | Other | WRITE | DENY | DENY | **PASS** |
| File | 755 | Other | EXEC | ALLOW | ALLOW | **PASS** |
| File | 755 | Root | READ | ALLOW | ALLOW | **PASS** |
| File | 755 | Root | WRITE | ALLOW | ALLOW | **PASS** |
| File | 755 | Root | EXEC | ALLOW | ALLOW | **PASS** |
| File | 4755 | Owner | READ | ALLOW | ALLOW | **PASS** |
| File | 4755 | Owner | WRITE | ALLOW | ALLOW | **PASS** |
| File | 4755 | Owner | EXEC | ALLOW | ALLOW | **PASS** |
| File | 4755 | Group | READ | ALLOW | ALLOW | **PASS** |
| File | 4755 | Group | WRITE | DENY | DENY | **PASS** |
| File | 4755 | Group | EXEC | ALLOW | ALLOW | **PASS** |
| File | 4755 | Other | READ | ALLOW | ALLOW | **PASS** |
| File | 4755 | Other | WRITE | DENY | DENY | **PASS** |
| File | 4755 | Other | EXEC | ALLOW | ALLOW | **PASS** |
| File | 4755 | Root | READ | ALLOW | ALLOW | **PASS** |
| File | 4755 | Root | WRITE | ALLOW | ALLOW | **PASS** |
| File | 4755 | Root | EXEC | ALLOW | ALLOW | **PASS** |
| Directory | 000 | Owner | SEARCH | DENY | DENY | **PASS** |
| Directory | 000 | Owner | CREATE | DENY | DENY | **PASS** |
| Directory | 000 | Owner | DELETE | DENY | DENY | **PASS** |
| Directory | 000 | Group | SEARCH | DENY | DENY | **PASS** |
| Directory | 000 | Group | CREATE | DENY | DENY | **PASS** |
| Directory | 000 | Group | DELETE | DENY | DENY | **PASS** |
| Directory | 000 | Other | SEARCH | DENY | DENY | **PASS** |
| Directory | 000 | Other | CREATE | DENY | DENY | **PASS** |
| Directory | 000 | Other | DELETE | DENY | DENY | **PASS** |
| Directory | 000 | Root | SEARCH | DENY | DENY | **PASS** |
| Directory | 000 | Root | CREATE | DENY | DENY | **PASS** |
| Directory | 000 | Root | DELETE | DENY | DENY | **PASS** |
| Directory | 600 | Owner | SEARCH | DENY | DENY | **PASS** |
| Directory | 600 | Owner | CREATE | DENY | DENY | **PASS** |
| Directory | 600 | Owner | DELETE | DENY | DENY | **PASS** |
| Directory | 600 | Group | SEARCH | DENY | DENY | **PASS** |
| Directory | 600 | Group | CREATE | DENY | DENY | **PASS** |
| Directory | 600 | Group | DELETE | DENY | DENY | **PASS** |
| Directory | 600 | Other | SEARCH | DENY | DENY | **PASS** |
| Directory | 600 | Other | CREATE | DENY | DENY | **PASS** |
| Directory | 600 | Other | DELETE | DENY | DENY | **PASS** |
| Directory | 600 | Root | SEARCH | DENY | DENY | **PASS** |
| Directory | 600 | Root | CREATE | DENY | DENY | **PASS** |
| Directory | 600 | Root | DELETE | DENY | DENY | **PASS** |
| Directory | 640 | Owner | SEARCH | DENY | DENY | **PASS** |
| Directory | 640 | Owner | CREATE | DENY | DENY | **PASS** |
| Directory | 640 | Owner | DELETE | DENY | DENY | **PASS** |
| Directory | 640 | Group | SEARCH | DENY | DENY | **PASS** |
| Directory | 640 | Group | CREATE | DENY | DENY | **PASS** |
| Directory | 640 | Group | DELETE | DENY | DENY | **PASS** |
| Directory | 640 | Other | SEARCH | DENY | DENY | **PASS** |
| Directory | 640 | Other | CREATE | DENY | DENY | **PASS** |
| Directory | 640 | Other | DELETE | DENY | DENY | **PASS** |
| Directory | 640 | Root | SEARCH | DENY | DENY | **PASS** |
| Directory | 640 | Root | CREATE | DENY | DENY | **PASS** |
| Directory | 640 | Root | DELETE | DENY | DENY | **PASS** |
| Directory | 644 | Owner | SEARCH | DENY | DENY | **PASS** |
| Directory | 644 | Owner | CREATE | DENY | DENY | **PASS** |
| Directory | 644 | Owner | DELETE | DENY | DENY | **PASS** |
| Directory | 644 | Group | SEARCH | DENY | DENY | **PASS** |
| Directory | 644 | Group | CREATE | DENY | DENY | **PASS** |
| Directory | 644 | Group | DELETE | DENY | DENY | **PASS** |
| Directory | 644 | Other | SEARCH | DENY | DENY | **PASS** |
| Directory | 644 | Other | CREATE | DENY | DENY | **PASS** |
| Directory | 644 | Other | DELETE | DENY | DENY | **PASS** |
| Directory | 644 | Root | SEARCH | DENY | DENY | **PASS** |
| Directory | 644 | Root | CREATE | DENY | DENY | **PASS** |
| Directory | 644 | Root | DELETE | DENY | DENY | **PASS** |
| Directory | 755 | Owner | SEARCH | ALLOW | ALLOW | **PASS** |
| Directory | 755 | Owner | CREATE | ALLOW | ALLOW | **PASS** |
| Directory | 755 | Owner | DELETE | ALLOW | ALLOW | **PASS** |
| Directory | 755 | Group | SEARCH | ALLOW | ALLOW | **PASS** |
| Directory | 755 | Group | CREATE | DENY | DENY | **PASS** |
| Directory | 755 | Group | DELETE | DENY | DENY | **PASS** |
| Directory | 755 | Other | SEARCH | ALLOW | ALLOW | **PASS** |
| Directory | 755 | Other | CREATE | DENY | DENY | **PASS** |
| Directory | 755 | Other | DELETE | DENY | DENY | **PASS** |
| Directory | 755 | Root | SEARCH | ALLOW | ALLOW | **PASS** |
| Directory | 755 | Root | CREATE | ALLOW | ALLOW | **PASS** |
| Directory | 755 | Root | DELETE | ALLOW | ALLOW | **PASS** |
| Directory | 4755 | Owner | SEARCH | ALLOW | ALLOW | **PASS** |
| Directory | 4755 | Owner | CREATE | ALLOW | ALLOW | **PASS** |
| Directory | 4755 | Owner | DELETE | ALLOW | ALLOW | **PASS** |
| Directory | 4755 | Group | SEARCH | ALLOW | ALLOW | **PASS** |
| Directory | 4755 | Group | CREATE | DENY | DENY | **PASS** |
| Directory | 4755 | Group | DELETE | DENY | DENY | **PASS** |
| Directory | 4755 | Other | SEARCH | ALLOW | ALLOW | **PASS** |
| Directory | 4755 | Other | CREATE | DENY | DENY | **PASS** |
| Directory | 4755 | Other | DELETE | DENY | DENY | **PASS** |
| Directory | 4755 | Root | SEARCH | ALLOW | ALLOW | **PASS** |
| Directory | 4755 | Root | CREATE | ALLOW | ALLOW | **PASS** |
| Directory | 4755 | Root | DELETE | ALLOW | ALLOW | **PASS** |

## 7. Evidence

- Raw QEMU output: `../weekly/W06/evidence/permission-matrix-pass.log`
- The raw log contains all 144 individual test results and the final summary.
- Terminal screenshots should be added to `do../weekly/W06/evidence/`.

## 8. Design references

- [W01 test matrix](../weekly/W01/notes/test-matrix.md)
- [W02 permission specification](../weekly/W02/notes/spec-permission.md)
