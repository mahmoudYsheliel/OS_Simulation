#include "data_structures/pq.h"
#include "headers.h"
#include "shm_manager.h"

pq_heap* procs_q;
sem_t* shm_sem;
void* shm_ptr;
pid_t sch_fork_pid;
pid_t clk_fork_pid;
int finish_flag = 0;

void clearResources(int);

void int_sig_handler(int sig_code) {
  sys_print("statring main termination squence\n");
  clearResources(sig_code);
  sys_print("main termination squence finished\n");
  signal(SIGINT, SIG_DFL);
  raise(SIGINT);
}

int main(int argc, char* argv[]) {
  signal(SIGINT, int_sig_handler);
  // #region start_clk_sch_region
  sys_print(
      "enter schduling algorithm (1 -> PHPF, 2 -> SJF, 3 -> {RR TQ}, 4 -> "
      "SRTN): ");
  char usr_sch_alg[8];
  fgets(usr_sch_alg, 8, stdin);
  int temp_sch_alg;
  char rr_tq[8];
  sscanf(usr_sch_alg, "%d", &temp_sch_alg);
  if (temp_sch_alg == 3) {
    sys_print("enter time quanta for the RR scheduling algorithm: ");
    char rr_tq[8];
    fgets(rr_tq, 8, stdin);
  }
  sys_print("starting clock process\n");
  clk_fork_pid = fork();
  if (clk_fork_pid == 0) {
    execl("./clk", "./clk", NULL);
  } else {
    sys_print("system starting scheduler process\n");
    sch_fork_pid = fork();
    if (sch_fork_pid == 0) {
      if (temp_sch_alg != 3) {
        execl("./scheduler", "./scheduler", usr_sch_alg, NULL);
      } else {
        execl("./scheduler", "./scheduler", usr_sch_alg, rr_tq, NULL);
      }
    } else {
      // process_generator main_code
      sys_print("process_generator entering main section\n");
      // #region read_file_region
      FILE* p_file;
      char inp_buffer[255];
      p_file = fopen("./processes_data.txt", "r");
      procs_q = malloc(sizeof(pq_heap));
      while (fgets(inp_buffer, 255, p_file) != NULL) {
        if (inp_buffer[0] != '#') {
          int p_id, p_arr_time, p_run_time, p_pror, mem_size;
          sscanf(inp_buffer, "%d %d %d %d %d", &p_id, &p_arr_time, &p_run_time,
                 &p_pror, &mem_size);
          process_data tmp_obj;
          tmp_obj.id = p_id;
          tmp_obj.arr_time = p_arr_time;
          tmp_obj.run_time = p_run_time;
          tmp_obj.pror = p_pror;
          tmp_obj.mem_size = mem_size;
          pq_push(procs_q, p_arr_time, &tmp_obj);
        }
      }
      sem_unlink("shm_sem");
      shm_sem = sem_open("shm_sem", O_CREAT, 0666, 1);
      if (shm_sem == SEM_FAILED) {
        sys_print("can't create shm semphore\n");
        clearResources(0);
        exit(1);
      }
      shm_ptr = attach_shm(PROCS_SHM);
      if (shm_ptr == NULL) {
        sys_print("can't create a shared memory region\n");
        clearResources(0);
        exit(1);
      }
      // #endregion

      initClk();
      // #region main_loop_region
      while (1) {
        if (!finish_flag) {
          int cur_tik = getClk();
          if (pq_top(procs_q)->arr_time <= cur_tik) {
            process_data* temp_proc = pq_pop(procs_q);
            sem_wait(shm_sem);
            memcpy(shm_ptr, temp_proc, sizeof(process_data));
            kill(sch_fork_pid, SIGINT);
            free(temp_proc);
          }
          if (procs_q->len == 0) {
            *(((char*)shm_ptr) + (SHM_SIZE - 1)) = 1;
            finish_flag = 1;
            sys_print("all process sent to the scheduler\n");
          }
          sleep(0.1f);
        } else {
          sleep(1);
        }
      }
      // #endregion
    }
  }
}

void clearResources(int signum) {
  sys_print("killing subprocesses\n");
  kill(sch_fork_pid, SIGKILL);
  kill(clk_fork_pid, SIGINT);
  sys_print("freeing IPC resources\n");
  destroyClk();
  sem_unlink("shm_sem");
  shm_daloc(PROCS_SHM);
}