#ifndef PQ
#define PQ

#include <stdlib.h>
#include "custom_structs.h"

typedef struct pq_node {
  int prror;
  process_data data;
} pq_node;

typedef struct pq_heap {
  pq_node* nodes;
  int len;
  int size;
} pq_heap;

void pq_push(pq_heap* h, int priority, process_data* data) {
  if (h->len + 1 >= h->size) {
    h->size = h->size ? h->size * 2 : 4;
    h->nodes = (pq_node*)realloc(h->nodes, h->size * sizeof(pq_node));
  }
  int i = h->len + 1;
  int j = i / 2;
  while (i > 1 && h->nodes[j].prror > priority) {
    h->nodes[i] = h->nodes[j];
    i = j;
    j = j / 2;
  }
  h->nodes[i].prror = priority;
  h->nodes[i].data.id = data->id;
  h->nodes[i].data.arr_time = data->arr_time;
  h->nodes[i].data.run_time = data->run_time;
  h->nodes[i].data.pror = data->pror;
  h->nodes[i].data.remain_time = data->remain_time;
  h->nodes[i].data.total_wait = data->total_wait;
  h->nodes[i].data.system_pid = data->system_pid;
  h->nodes[i].data.mem_size = data->mem_size;
  h->len++;
}

process_data* pq_pop(pq_heap* h) {
  int i, j, k;
  if (!h->len) {
    return NULL;
  }
  process_data* temp_data = (process_data*)malloc(sizeof(process_data));
  temp_data->id = h->nodes[1].data.id;
  temp_data->arr_time = h->nodes[1].data.arr_time;
  temp_data->run_time = h->nodes[1].data.run_time;
  temp_data->pror = h->nodes[1].data.pror;
  temp_data->remain_time = h->nodes[1].data.remain_time;
  temp_data->total_wait = h->nodes[1].data.total_wait;
  temp_data->system_pid = h->nodes[1].data.system_pid;
  temp_data->mem_size = h->nodes[1].data.mem_size;
  h->nodes[1] = h->nodes[h->len];
  h->len--;
  i = 1;
  while (i != h->len + 1) {
    k = h->len + 1;
    j = 2 * i;
    if (j <= h->len && h->nodes[j].prror < h->nodes[k].prror) {
      k = j;
    }
    if (j + 1 <= h->len && h->nodes[j + 1].prror < h->nodes[k].prror) {
      k = j + 1;
    }
    h->nodes[i] = h->nodes[k];
    i = k;
  }
  return temp_data;
}

process_data* pq_top(pq_heap* h) {
  if (h->len <= 0) {
    return NULL;
  }
  return &h->nodes[1].data;
}

#endif