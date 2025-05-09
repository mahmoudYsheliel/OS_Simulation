#include "headers.h"
#include "shm_manager.h"

int remainingtime;

void* shm_ptr;

int main(int agrc, char* argv[]) {
  initClk();
  shm_ptr = attach_shm(PROCS_SHM);
  if (shm_ptr == NULL) {
    printf("can't attach procs' shared memory region\n");
    exit(1);
  }
  int pid, run_time;
  sscanf(argv[1], "%d", &pid);
  sscanf(argv[2], "%d", &run_time);
  char temp_buf_2[64];
  sprintf(temp_buf_2, "process %d started\n", pid);
  sys_print(temp_buf_2);
  remainingtime = run_time;
  while (remainingtime > 0) {
    remainingtime--;
    char temp_buf_1[64];
    sprintf(temp_buf_1, "process %d running remaining time %d\n", pid,
            remainingtime);
    sys_print(temp_buf_1);
    sleep(1);
  }
  destroyClk();
  // set child signal flag
  *(((char*)shm_ptr) + (SHM_SIZE - 2)) = 1;
  exit(pid);
}
