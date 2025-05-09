#ifndef MEM_MANAGER
#define MEM_MANAGER

#include <math.h>

#include "./data_structures/custom_structs.h"

#define SYS_MEM 1024

// struct that contains list of memeory segments of the same size
typedef struct mem_pool {
  int type;
  int count;
  mem_segment* segs_pool;
} mem_pool;

// os data structure containes list of all memory segments
// memory segmnets are categorized by their size
mem_pool os_mem_pool[7];

// api function: prepare memory managment system data structures
void init_memory() {
  for (int i = 0; i < 7; i++) {
    mem_pool temp_obj;
    temp_obj.type = 16 * pow(2, i);
    int max_no_segs =
        floor((float)SYS_MEM / (float)(temp_obj.type + temp_obj.type * 2));
    temp_obj.segs_pool =
        (mem_segment*)malloc(sizeof(mem_segment) * max_no_segs);
    // os have initaly 1KB memory
    if (i == 6) {
      temp_obj.count = 1;
      temp_obj.segs_pool[0].begin_addr = 0;
      temp_obj.segs_pool[0].size = SYS_MEM;
    } else {
      temp_obj.count = 0;
    }
    os_mem_pool[i] = temp_obj;
  }
}

// internal function: perform buddy fragmentaion
int mem_split(int _size) {
  if (_size == (SYS_MEM * 2)) {
    return -1;
  }
  int cont_block = _size * 2;  // the bigger block size
  int size_log = (int)log2(cont_block);
  int needed_seg_index = size_log - 4;
  if (os_mem_pool[needed_seg_index].count == 0) {
    int ret_code = mem_split(cont_block);
    if (ret_code == -1) {
      return -1;
    }
  }
  os_mem_pool[needed_seg_index].count--;
  int bigger_addr = os_mem_pool[needed_seg_index]
                        .segs_pool[os_mem_pool[needed_seg_index].count]
                        .begin_addr;

  os_mem_pool[needed_seg_index - 1].segs_pool[0].begin_addr = bigger_addr;
  os_mem_pool[needed_seg_index - 1].segs_pool[0].size = _size;

  os_mem_pool[needed_seg_index - 1].segs_pool[1].begin_addr =
      bigger_addr + _size;
  os_mem_pool[needed_seg_index - 1].segs_pool[1].size = _size;

  os_mem_pool[needed_seg_index - 1].count += 2;
  return 1;
}

// api function: allocate memory size
mem_segment mem_alloc(int size) {
  int size_log = (int)ceil(log2(size));
  int needed_seg_index = size_log - 4;
  if (os_mem_pool[needed_seg_index].count <= 0) {
    mem_split((int)pow(2, size_log));
  }
  // return apporpiate segment
  os_mem_pool[needed_seg_index].count--;
  return os_mem_pool[needed_seg_index]
      .segs_pool[os_mem_pool[needed_seg_index].count];
}

// internal function: get memory segment data by address
mem_segment get_seg_by_addr(int beg_addr, int size) {
  int pool_index = (int)ceil(log2(size)) - 4;
  mem_segment out_seg;
  out_seg.begin_addr = -1;
  out_seg.size = 0;
  for (int i = 0; i < os_mem_pool[pool_index].count; i++) {
    if (os_mem_pool[pool_index].segs_pool[i].begin_addr == beg_addr) {
      out_seg.begin_addr = os_mem_pool[pool_index].segs_pool[i].begin_addr;
      out_seg.size = os_mem_pool[pool_index].segs_pool[i].size;
      break;
    }
  }
  return out_seg;
}

// internal function: fix pool sructure after merge operation
void fix_bool(int pool_index, int seg1_beg_addr, int seg_size) {
  int seg2_beg_addr = seg1_beg_addr + seg_size;
  int old_count = os_mem_pool[pool_index].count;
  os_mem_pool[pool_index].count = 0;
  for (int i = 0, j = 0; i < old_count; i++) {
    if (os_mem_pool[pool_index].segs_pool[i].begin_addr != seg1_beg_addr &&
        os_mem_pool[pool_index].segs_pool[i].begin_addr != seg2_beg_addr) {
      os_mem_pool[pool_index].segs_pool[j].begin_addr =
          os_mem_pool[pool_index].segs_pool[i].begin_addr;
      os_mem_pool[pool_index].segs_pool[j].size =
          os_mem_pool[pool_index].segs_pool[i].size;
      j++;
      os_mem_pool[pool_index].count++;
    }
  }
}

// internal function: buddy merge memory segments
void scan_and_merge() {
  for (int i = 0; i < 7; i++) {
    if (os_mem_pool[i].count > 1) {
      for (int j = 0; j < os_mem_pool[i].count; j++) {
        // checks if the beginig address is even multiable of the size to merge
        int size_coof = os_mem_pool[i].segs_pool[j].begin_addr /
                        os_mem_pool[i].segs_pool[j].size;
        if (size_coof % 2 == 0) {
          // get segment to merge with
          int merge_address =
              (size_coof + 1) * os_mem_pool[i].segs_pool[j].size;
          mem_segment seg_to_merge =
              get_seg_by_addr(merge_address, os_mem_pool[i].segs_pool[j].size);
          if (seg_to_merge.begin_addr != -1) {
            fix_bool(i, os_mem_pool[i].segs_pool[j].begin_addr,
                     os_mem_pool[i].segs_pool[j].size);
            os_mem_pool[i + 1].segs_pool[os_mem_pool[i + 1].count].begin_addr =
                os_mem_pool[i].segs_pool[j].begin_addr;
            os_mem_pool[i + 1].segs_pool[os_mem_pool[i + 1].count].size =
                os_mem_pool[i].segs_pool[j].size * 2;
            os_mem_pool[i + 1].count++;
          }
        }
      }
    }
  }
}

// api function: delete allocated segment
void mem_dalloc(mem_segment segment) {
  int size_log = (int)ceil(log2(segment.size));
  int pool_index = size_log - 4;
  // add memory segment again to the system
  os_mem_pool[pool_index].segs_pool[os_mem_pool[pool_index].count].begin_addr =
      segment.begin_addr;
  os_mem_pool[pool_index].segs_pool[os_mem_pool[pool_index].count].size =
      segment.size;
  os_mem_pool[pool_index].count++;
  // buddy merge the new memory structure
  scan_and_merge();
}

#endif