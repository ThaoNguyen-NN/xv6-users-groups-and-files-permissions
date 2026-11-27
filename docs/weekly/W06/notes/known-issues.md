# W06 - Known Issues and Compatibility Notes

## 1. Metadata persistence

The UGP-35 persistence test verifies that mode, uid, and gid
remain unchanged after a normal reboot using the same fs.img.

Verified test objects:

| Object | Mode | UID | GID |
|---|---:|---:|---:|
| Regular file | 0640 | 1000 | 100 |
| Directory | 0750 | 1002 | 200 |

Result: PASS.

## 2. Hard-link compatibility

The test creates a hard link to a file and verifies metadata
through both pathnames.

- Metadata remains unchanged after link creation.
- Removing the original pathname does not remove the inode
  while another hard link still exists.
- Metadata remains accessible through the remaining pathname.

Result: PASS.

## 3. Truncate compatibility

The test opens the file with O_TRUNC and verifies that:

- File size becomes zero.
- Mode remains 0640.
- UID remains 1000.
- GID remains 100.
- Link count remains one.

Result: PASS.

## 4. Crash recovery

The existing crash test suite completed successfully:

- Log recovery: OK.
- Orphaned file recovery: OK.
- Orphaned directory recovery: OK.

These tests verify existing filesystem recovery behavior.
They do not specifically simulate a crash at the exact moment
chmod or chown is updating inode metadata.

## 5. Remaining limitations

No metadata loss was reproduced in the tested link, unlink,
truncate, or normal reboot scenarios.

A crash occurring at every possible point during chmod/chown
has not been exhaustively tested.

Further fault-injection testing would be needed to establish
behavior for all interruption timings.
