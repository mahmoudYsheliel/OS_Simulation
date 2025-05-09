#ifndef SHM_MANAGER
#define SHM_MANAGER

#include <stdlib.h>
#include <sys/shm.h>
#include <sys/types.h>

#define SHM_SIZE 64

void* attach_shm(int shm_key) {
  int shm_id = shmget(shm_key, SHM_SIZE, 0666 | IPC_CREAT);
  if (shm_id == -1) {
    return NULL;
  } else {
    return shmat(shm_id, NULL, 0);
  }
}

int datach_shm(void* mem_ptr) {
  return (shmdt(mem_ptr) != -1);
}

int shm_daloc(int shm_key) {
  int shm_id = shmget(shm_key, SHM_SIZE, 0666 | IPC_CREAT);
  return shmctl(shm_id, IPC_RMID, NULL);
}

#endif