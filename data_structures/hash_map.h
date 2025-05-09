#ifndef HASH_MAP
#define HASH_MAP

#include <stdlib.h>
#include "custom_structs.h"

typedef struct node {
  int key;
  process_data* val;
  struct node* next;
} node;

typedef struct table {
  int size;
  struct node** list;
} table;

struct table* createTable(int size) {
  struct table* t = (struct table*)malloc(sizeof(struct table));
  t->size = size;
  t->list = (struct node**)malloc(sizeof(struct node*) * size);
  int i;
  for (i = 0; i < size; i++)
    t->list[i] = NULL;
  return t;
}

int hashCode(struct table* t, int key) {
  if (key < 0)
    return -(key % t->size);
  return key % t->size;
}
void hash_map_insert(struct table* t, int key, process_data* val) {
  int pos = hashCode(t, key);
  struct node* list = t->list[pos];
  struct node* newNode = (struct node*)malloc(sizeof(struct node));
  struct node* temp = list;
  while (temp) {
    if (temp->key == key) {
      temp->val->id = val->id;
      return;
    }
    temp = temp->next;
  }
  newNode->key = key;
  newNode->val = val;
  newNode->next = list;
  t->list[pos] = newNode;
}

process_data* lookup(struct table* t, int key) {
  int pos = hashCode(t, key);
  struct node* list = t->list[pos];
  struct node* temp = list;
  while (temp) {
    if (temp->key == key) {
      return temp->val;
    }
    temp = temp->next;
  }
  return NULL;
}

#endif