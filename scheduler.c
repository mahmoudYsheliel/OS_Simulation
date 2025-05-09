#include <math.h>
#include "data_structures/array_list.h"
#include "data_structures/hash_map.h"
#include "data_structures/pq.h"
#include "headers.h"
#include "mem_manager.h"
#include "shm_manager.h"

// IPC resources
sem_t* shm_sem;
void* shm_ptr;

// system data structures
pq_heap* procs_q;        // used by HPF and SJF
array_list* procs_list;  // user by round roubin
table* pcb_table;

// global variables
int sch_alg;
int log_line_index = 0;
// HPF SJF
int is_run_flag = 0;  // indicates if scheduler is currnetly running a process
// RR
int rr_proc_index = 0;   // process index in RR
int fin_proc_count = 0;  // nunmer of finished processes
#define MOD(a, b) ((((a) % (b)) + (b)) % (b))

int proc_big_time = -1;             // RR|SRTN process begin time
process_data* cur_run_proc = NULL;  // RR|SRTN process begin pointer

// SRTN
int preampt_flag = 0;

// output files
FILE* sch_logs;
FILE* perf_logs;
FILE* mem_logs;

// statistics variables
int total_run_time = 0;

void int_sig_handler(int sig_code) {
  char temp_buf[SHM_SIZE];
  memcpy(temp_buf, shm_ptr, sizeof(process_data));
  process_data* temp_process = malloc(sizeof(process_data));
  process_data* temp_buf_proc = (process_data*)temp_buf;
  total_run_time += temp_buf_proc->run_time;
  temp_process->id = temp_buf_proc->id;
  temp_process->arr_time = temp_buf_proc->arr_time;
  temp_process->pror = temp_buf_proc->pror;
  temp_process->remain_time = temp_buf_proc->remain_time;
  temp_process->run_time = temp_buf_proc->run_time;
  temp_process->total_wait = temp_buf_proc->total_wait;
  temp_process->remain_time = temp_process->run_time;
  // <phase_2_memory managment>
  temp_process->mem_size = temp_buf_proc->mem_size;
  mem_segment temp_seg = mem_alloc(temp_process->mem_size);
  temp_process->system_mem.begin_addr = temp_seg.begin_addr;
  temp_process->system_mem.size = temp_seg.size;
  char log_line[128];
  sprintf(log_line,
          "At\ttime\t%d\tallocated\t%d\tbytes\tfor\tprocess\t%d\tfrom\t%"
          "d\tto\t%d\n",
          getClk(), temp_process->mem_size, temp_process->id,
          temp_process->system_mem.begin_addr,
          temp_process->system_mem.begin_addr + temp_process->system_mem.size);
  sys_print("[LOG]: ");
  sys_print(log_line);
  fputs(log_line, mem_logs);
  // </phase_2_memory managment>
  temp_process->total_wait = 0;
  temp_process->system_pid = -1;
  hash_map_insert(pcb_table, temp_process->id, temp_process);
  char temp_buf_1[64];
  sprintf(temp_buf_1, "[TICK:%d]: recived process id: %d\n", getClk(),
          temp_process->id);
  sys_print(temp_buf_1);
  switch (sch_alg) {
    case 1:
      // HPF init
      if (cur_run_proc != NULL && temp_process->pror < cur_run_proc->pror) {
        preampt_flag = 1;
      }
      pq_push(procs_q, temp_process->pror, temp_process);
      break;
    case 2:
      // SJF init
      pq_push(procs_q, temp_process->run_time, temp_process);
      break;
    case 3:
      // RR init
      insert_array(procs_list, temp_process);
      break;
    case 4:
      // SRTN init
      if (cur_run_proc != NULL &&
          temp_process->run_time < cur_run_proc->run_time) {
        preampt_flag = 1;
      }
      pq_push(procs_q, temp_process->run_time, temp_process);
      break;
  }
  sem_post(shm_sem);
}

void sig_child_handler(int sig_code) {
  int chld_flag = *(((char*)shm_ptr) + (SHM_SIZE - 2));
  if (chld_flag != 1) {
    return;
  }
  *(((char*)shm_ptr) + (SHM_SIZE - 2)) = 0;
  int c_es = -1;
  wait(&c_es);
  if (WIFEXITED(c_es)) {
    int c_ec = WEXITSTATUS(c_es);
    char temp_buf_1[64];
    sprintf(temp_buf_1, "process %d finished\n", c_ec);
    sys_print(temp_buf_1);
    process_data* proc = lookup(pcb_table, c_ec);
    if (sch_alg == 1) {
      // HPF process exit sequence
      is_run_flag = 0;
    } else if (sch_alg == 2) {
      // SJF process exit sequence
      is_run_flag = 0;
    } else if (sch_alg == 3) {
      // RR process exit sequence
      proc->system_pid = -2;
      fin_proc_count++;
      proc_big_time = -1;
      int stop_flag = *(((char*)shm_ptr) + (SHM_SIZE - 1));
      if (stop_flag == 1 && fin_proc_count == procs_list->count) {
        free_array(procs_list);
      }
    } else if (sch_alg == 4) {
      // SRTN termination sequence
      is_run_flag = 0;
    }
    char log_line[128];
    int ta = getClk() - proc->arr_time;
    float wta = (float)ta / (float)proc->run_time;
    int proc_wait_time = ta - proc->run_time;
    proc->total_wait = proc_wait_time;
    sprintf(log_line,
            "At\ttime\t%d\tprocess\t%d\t%s\tarr\t%d\ttotal\t%d\tremain\t%d"
            "\twait\t"
            "%d TA %d WTA %.2f\n",
            getClk(), proc->id, "finished", proc->arr_time, proc->run_time, 0,
            proc_wait_time, ta, wta);
    sys_print("[LOG]: ");
    sys_print(log_line);
    fputs(log_line, sch_logs);
    // <phase_2_memory managment>
    mem_dalloc(proc->system_mem);
    sys_print("[LOG]: ");
    sprintf(
        log_line,
        "At\ttime\t%d\tfreed\t%d\tbytes\tfrom\tprocess\t%d\tfrom\t%"
        "d\tto\t%d\n",
        getClk(), proc->mem_size, proc->id,
        proc->system_mem.begin_addr,
        proc->system_mem.begin_addr + proc->system_mem.size);
    sys_print(log_line);
    fputs(log_line, mem_logs);
    // </phase_2_memory managment>
  }
}

int main(int argc, char* argv[]) {
  signal(SIGINT, int_sig_handler);
  signal(SIGCHLD, sig_child_handler);
  sch_logs = fopen("../output_logs/Scheduler.log", "w");
  perf_logs = fopen("../output_logs/scheduler.perf", "w");
  mem_logs = fopen("../output_logs/memory.log", "w");
  initClk();
  // <phase_2_memory managment>
  init_memory();
  // </phase_2_memory managment>
  char init_log_line[128];
  sprintf(init_log_line,
          "#At\ttime\tx\tprocess\ty\tstate\tarr\tw\ttotal\tz\tremain\ty\twait\t"
          "k\n");
  fputs(init_log_line, sch_logs);
  sprintf(
      init_log_line,
      "#At\ttime\tx\tallocated\ty\tbytes\tfor\tprocess\tz\tfrom\ti\tto\tj\n");
  fputs(init_log_line, mem_logs);
  procs_q = (pq_heap*)malloc(sizeof(pq_heap));
  procs_list = (array_list*)malloc(sizeof(array_list));
  pcb_table = createTable(5);
  sscanf(argv[1], "%d", &sch_alg);
  shm_sem = sem_open("shm_sem", 0);
  if (shm_sem == SEM_FAILED) {
    sys_print("can't open shm semphore\n");
    exit(1);
  }
  shm_ptr = attach_shm(PROCS_SHM);
  if (shm_ptr == NULL) {
    sys_print("can't attach procs' shared memory region\n");
    exit(1);
  }

  if (sch_alg == 1) {
    // PHPF
    sys_print("scheduler porcess stated with algorithm PHPF\n");
    while (1) {
      sleep(0.1f);
      if (procs_q->len == 0) {
        int stop_flag = *(((char*)shm_ptr) + (SHM_SIZE - 1));
        if (stop_flag == 1 && !is_run_flag) {
          break;
        } else {
          continue;
        }
      } else {
        if (!is_run_flag) {
          is_run_flag = 1;
          process_data* proc = pq_pop(procs_q);
          cur_run_proc = lookup(pcb_table, proc->id);
          char log_line[128];
          if (proc->system_pid == -1) {
            sprintf(
                log_line,
                "At\ttime\t%d\tprocess\t%d\t%s\tarr\t%d\ttotal\t%d\tremain\t%d"
                "\twait\t"
                "%d\n",
                getClk(), proc->id, "started", proc->arr_time, proc->run_time,
                proc->remain_time, proc->total_wait);
            sys_print("[LOG]: ");
            sys_print(log_line);
            fputs(log_line, sch_logs);
            proc_big_time = getClk();
            pid_t child_proc = fork();
            if (child_proc == 0) {
              char proc_arg_1[4];
              char proc_arg_2[4];
              sprintf(proc_arg_1, "%d", proc->id);
              sprintf(proc_arg_2, "%d", proc->run_time);
              execl("./process", "./process", proc_arg_1, proc_arg_2, NULL);
            }
            cur_run_proc->system_pid = child_proc;
            free(proc);
          } else {
            sprintf(
                log_line,
                "At\ttime\t%d\tprocess\t%d\t%s\tarr\t%d\ttotal\t%d\tremain\t%d"
                "\twait\t"
                "%d\n",
                getClk(), proc->id, "resumed", proc->arr_time, proc->run_time,
                proc->remain_time, proc->total_wait);
            fputs(log_line, sch_logs);
            kill(proc->system_pid, SIGCONT);
          }
        } else {
          if (preampt_flag == 1) {
            preampt_flag = 0;
            kill(cur_run_proc->system_pid, SIGSTOP);
            char log_line[128];
            sprintf(
                log_line,
                "At\ttime\t%d\tprocess\t%d\t%s\tarr\t%d\ttotal\t%d\tremain\t%d"
                "\twait\t"
                "%d\n",
                getClk(), cur_run_proc->id, "stopped", cur_run_proc->arr_time,
                cur_run_proc->run_time, cur_run_proc->remain_time,
                cur_run_proc->total_wait);
            sys_print("[LOG]: ");
            sys_print(log_line);
            fputs(log_line, sch_logs);
            cur_run_proc->remain_time -= (getClk() - proc_big_time);
            is_run_flag = 0;
            pq_push(procs_q, cur_run_proc->pror, cur_run_proc);
          }
        }
      }
    }
  } else if (sch_alg == 2) {
    // SJF scheduling
    sys_print("scheduler porcess stated with algorithm SJF\n");
    while (1) {
      sleep(0.1f);
      if (procs_q->len == 0) {
        int stop_flag = *(((char*)shm_ptr) + (SHM_SIZE - 1));
        if (stop_flag == 1 && !is_run_flag) {
          break;
        } else {
          continue;
        }
      } else {
        if (!is_run_flag) {
          is_run_flag = 1;
          process_data* proc = pq_pop(procs_q);
          char log_line[128];
          sprintf(
              log_line,
              "At\ttime\t%d\tprocess\t%d\t%s\tarr\t%d\ttotal\t%d\tremain\t%d"
              "\twait\t"
              "%d\n",
              getClk(), proc->id, "started", proc->arr_time, proc->run_time,
              proc->remain_time, proc->total_wait);
          sys_print("[LOG]: ");
          sys_print(log_line);
          fputs(log_line, sch_logs);
          pid_t child_proc = fork();
          if (child_proc == 0) {
            char proc_arg_1[4];
            char proc_arg_2[4];
            sprintf(proc_arg_1, "%d", proc->id);
            sprintf(proc_arg_2, "%d", proc->run_time);
            execl("./process", "./process", proc_arg_1, proc_arg_2, NULL);
          }
          free(proc);
        } else {
          continue;
        }
      }
    }
  } else if (sch_alg == 3) {
    // RR scheduling
    int time_quanta;
    char temp_buf_1[64];
    sscanf(argv[2], "%d", &time_quanta);
    sprintf(temp_buf_1,
            "scheduler porcess stated with algorithm RR with TQ %d\n",
            time_quanta);
    sys_print(temp_buf_1);
    while (1) {
      sleep(0.1f);
      if (procs_list->count == 0) {
        int stop_flag = *(((char*)shm_ptr) + (SHM_SIZE - 1));
        if (stop_flag == 1) {
          break;
        } else {
          continue;
        }
      } else {
        if (proc_big_time == -1) {
          process_data* proc = procs_list->array[rr_proc_index];
          rr_proc_index = (rr_proc_index + 1) % procs_list->count;
          cur_run_proc = proc;
          if (proc->system_pid == -2) {
            continue;
          }
          if (proc->system_pid == -1) {
            char log_line[128];
            int cur_tik = getClk();
            proc_big_time = cur_tik;
            sprintf(
                log_line,
                "At\ttime\t%d\tprocess\t%d\t%s\tarr\t%d\ttotal\t%d\tremain\t%d"
                "\twait\t"
                "%d\n",
                cur_tik, proc->id, "started", proc->arr_time, proc->run_time,
                proc->remain_time, proc->total_wait);
            sys_print("[LOG]: ");
            sys_print(log_line);
            fputs(log_line, sch_logs);
            pid_t child_proc = fork();
            if (child_proc == 0) {
              char proc_arg_1[4];
              char proc_arg_2[4];
              sprintf(proc_arg_1, "%d", proc->id);
              sprintf(proc_arg_2, "%d", proc->run_time);
              execl("./process", "./process", proc_arg_1, proc_arg_2, NULL);
            }
            proc->system_pid = child_proc;
          } else {
            char log_line[128];
            int cur_tik = getClk();
            proc_big_time = cur_tik;
            sprintf(
                log_line,
                "At\ttime\t%d\tprocess\t%d\t%s\tarr\t%d\ttotal\t%d\tremain\t%d"
                "\twait\t"
                "%d\n",
                cur_tik, proc->id, "resumed", proc->arr_time, proc->run_time,
                proc->remain_time, proc->total_wait);
            sys_print("[LOG]: ");
            sys_print(log_line);
            fputs(log_line, sch_logs);
            kill(proc->system_pid, SIGCONT);
          }
        } else {
          if (getClk() >= (proc_big_time + time_quanta)) {
            kill(cur_run_proc->system_pid, SIGSTOP);
            char log_line_2[128];
            sprintf(
                log_line_2,
                "At\ttime\t%d\tprocess\t%d\t%s\tarr\t%d\ttotal\t%d\tremain\t%d"
                "\twait\t"
                "%d\n",
                getClk(), cur_run_proc->id, "stopped", cur_run_proc->arr_time,
                cur_run_proc->run_time, cur_run_proc->remain_time,
                cur_run_proc->total_wait);
            sys_print("[LOG]: ");
            sys_print(log_line_2);
            fputs(log_line_2, sch_logs);
            cur_run_proc->remain_time -= time_quanta;
            proc_big_time = -1;
          }
        }
      }
    }
  } else if (sch_alg == 4) {
    // SRTN scheduling
    sys_print("scheduler porcess stated with algorithm SRTN\n");
    while (1) {
      sleep(0.1f);
      if (procs_q->len == 0) {
        int stop_flag = *(((char*)shm_ptr) + (SHM_SIZE - 1));
        if (stop_flag == 1 && !is_run_flag) {
          break;
        } else {
          continue;
        }
      } else {
        if (!is_run_flag) {
          is_run_flag = 1;
          process_data* proc = pq_pop(procs_q);
          cur_run_proc = lookup(pcb_table, proc->id);
          char log_line[128];
          if (proc->system_pid == -1) {
            sprintf(
                log_line,
                "At\ttime\t%d\tprocess\t%d\t%s\tarr\t%d\ttotal\t%d\tremain\t%d"
                "\twait\t"
                "%d\n",
                getClk(), proc->id, "started", proc->arr_time, proc->run_time,
                proc->remain_time, proc->total_wait);
            sys_print("[LOG]: ");
            sys_print(log_line);
            fputs(log_line, sch_logs);
            proc_big_time = getClk();
            pid_t child_proc = fork();
            if (child_proc == 0) {
              char proc_arg_1[4];
              char proc_arg_2[4];
              sprintf(proc_arg_1, "%d", proc->id);
              sprintf(proc_arg_2, "%d", proc->run_time);
              execl("./process", "./process", proc_arg_1, proc_arg_2, NULL);
            }
            cur_run_proc->system_pid = child_proc;
            free(proc);
          } else {
            sprintf(
                log_line,
                "At\ttime\t%d\tprocess\t%d\t%s\tarr\t%d\ttotal\t%d\tremain\t%d"
                "\twait\t"
                "%d\n",
                getClk(), proc->id, "resumed", proc->arr_time, proc->run_time,
                proc->remain_time, proc->total_wait);
            fputs(log_line, sch_logs);
            kill(proc->system_pid, SIGCONT);
          }
        } else {
          if (preampt_flag == 1) {
            preampt_flag = 0;
            kill(cur_run_proc->system_pid, SIGSTOP);
            char log_line[128];
            sprintf(
                log_line,
                "At\ttime\t%d\tprocess\t%d\t%s\tarr\t%d\ttotal\t%d\tremain\t%d"
                "\twait\t"
                "%d\n",
                getClk(), cur_run_proc->id, "stopped", cur_run_proc->arr_time,
                cur_run_proc->run_time, cur_run_proc->remain_time,
                cur_run_proc->total_wait);
            sys_print("[LOG]: ");
            sys_print(log_line);
            fputs(log_line, sch_logs);
            cur_run_proc->remain_time -= (getClk() - proc_big_time);
            is_run_flag = 0;
            pq_push(procs_q, cur_run_proc->remain_time, cur_run_proc);
          }
        }
      }
    }
  }
  sys_print("starting scheduler termenation sequence\n");
  int all_wait_time = 0;
  float all_wta_time = 0;
  int all_procs_no = 0;
  float cpu_ut = 0.0f;
  // computing all_procs_run_time, all_wait_time, all_wta_time
  for (int i = 0; i < pcb_table->size; i++) {
    if (pcb_table->list[i] != NULL) {
      all_wait_time += pcb_table->list[i]->val->total_wait;
      all_wta_time += ((float)pcb_table->list[i]->val->total_wait +
                       (float)pcb_table->list[i]->val->run_time) /
                      (float)pcb_table->list[i]->val->run_time;
      all_procs_no++;
    }
  }
  float avg_wta = (float)all_wta_time / (float)all_procs_no;
  // computing cpu ut
  cpu_ut = (float)total_run_time / (float)getClk() * 100.0f;
  float all_std_wta = 0;
  // computing all_std_wta
  for (int i = 0; i < pcb_table->size; i++) {
    if (pcb_table->list[i] != NULL) {
      all_std_wta += pow((((float)pcb_table->list[i]->val->total_wait +
                           (float)pcb_table->list[i]->val->run_time) /
                          (float)pcb_table->list[i]->val->run_time) -
                             avg_wta,
                         2);
    }
  }
  float avg_wait = (float)all_wait_time / (float)all_procs_no;
  float std_wta = sqrt(all_std_wta / (float)all_procs_no);
  char perf_logs_out_buf[128];
  sprintf(perf_logs_out_buf,
          "CPU utilization = %.2f%%\nAvg WTA = %.2f\nAvg Waiting = %.2f\nStd "
          "WTA = %.2f",
          cpu_ut, avg_wta, avg_wait, std_wta);
  fputs(perf_logs_out_buf, perf_logs);
  fclose(sch_logs);
  fclose(perf_logs);
  fclose(mem_logs);
  sys_print("scheduler termenation sequence finished\n");
  kill(getppid(), SIGINT);
}