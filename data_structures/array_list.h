#ifndef ARRAY_LIST
#define ARRAY_LIST

#include <stdlib.h>
#include "custom_structs.h"

typedef struct array_list {
  process_data** array;
  int count;
  int size;
} array_list;

void init_array(array_list* a, int initial_size) {
  a->array = (process_data**)malloc(initial_size * sizeof(process_data*));
  a->count = 0;
  a->size = initial_size;
}

void insert_array(array_list* a, process_data* element) {
  if (a->count == a->size) {
    a->size *= 2;
    a->array =
        (process_data**)realloc(a->array, a->size * sizeof(process_data*));
  }
  a->array[a->count++] = element;
}

void free_array(array_list* a) {
  free(a->array);
  a->array = NULL;
  a->count = 0;
  a->size = 0;
}

#endif