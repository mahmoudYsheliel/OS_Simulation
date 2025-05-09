#ifndef SYSTEM_STRUCTS
#define SYSTEM_STRUCTS

// struct that contains data of a single memory segment
typedef struct mem_segment {
  int begin_addr;
  int size;
} mem_segment;

// struct that contains data of a single process
typedef struct process_data {
  int id;
  int arr_time;
  int run_time;
  int pror;
  int remain_time;
  int total_wait;
  pid_t system_pid;
  int mem_size;
  mem_segment system_mem;
} process_data;

#endif