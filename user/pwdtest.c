#include "kernel/types.h"
#include "user/user.h"

// TODO (Thư): kiểm tra tra cứu user/group (docs/design/api.md, mục "Test dự kiến").
// In kết quả giống uidtest: "PASS: <nội dung>" hoặc "FAIL: <nội dung>" cho từng ca,
// dòng cuối "pwdtest: ALL PASSED" hoặc "pwdtest: FAILED".

// cậu sửa xong nhớ xóa đống note này đi nhé 

int
main(void)
{
  printf("FAIL: pwdtest not implemented\n");
  printf("pwdtest: FAILED\n");
  exit(1);
}
