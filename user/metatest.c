#include "kernel/types.h"
#include "user/user.h"

// TODO (Đức Anh): kiểm tra mode/uid/gid của file qua stat
// In kết quả giống uidtest: "PASS: <nội dung>" hoặc "FAIL: <nội dung>" cho từng ca,
// dòng cuối "pwdtest: ALL PASSED" hoặc "pwdtest: FAILED".

// cậu sửa xong nhớ xóa mấy cmt này đi nhé 

int
main(void)
{
  printf("FAIL: pwdtest not implemented\n");
  printf("metatest: FAILED\n");
  exit(1);
}
