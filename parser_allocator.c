#include <stdlib.h>
#include <stdint.h>
#include "parser_allocator.h"

#define HEAP_SIZE 4096
#define HEAP_OBJS 256
#define HEAP_GAP_MAX 8

typedef struct {
  uint16_t objType;
  size_t size;
  void* ptr;
} objHeader;

typedef struct {
  size_t size;
  void* storage;
  void* storageHWM;  // HWM: High Water Mark

  size_t headers_size;
  objHeader* headers;
  objHeader* headersHWM;
} heap;

typedef struct {
  pool public;
  
  size_t numHeaps;
  heap* heaps;
} poolImpl;

heap* pool_addHeap(poolImpl* this, size_t minSize){
  this->heaps = realloc(this->heaps, sizeof(heap) * (this->numHeaps + 1));
  heap* heap = (this->heaps + this->numHeaps);
  this->numHeaps++;

  heap->size = HEAP_SIZE;
  if(heap->size < minSize) heap->size = minSize;
  heap->storage = malloc(heap->size);
  heap->storageHWM = heap->storage;

  heap->headers_size = HEAP_OBJS;
  heap->headers = malloc(heap->headers_size * sizeof(objHeader));
  heap->headersHWM = heap->headers;

  return heap;
}
heap* pool_findVacant(poolImpl* this, poolObjType type, size_t size){
  if(this->heaps){
    heap* h;
    for(h = this->heaps; h < this->heaps + this->numHeaps; h++){
      if(h->size - (h->storageHWM - h->storage) < size) continue;
      if(h->headers_size - (h->headersHWM - h->headers) < 1) continue;
      
      return h;
    }
  }
  return pool_addHeap(this, size);
}
void* pool_alloc(pool* thisP, poolObjType type, size_t size){
  poolImpl* this = (poolImpl*)thisP;
  
  heap* h = pool_findVacant(this, type, size + HEAP_GAP_MAX);
  
  void* ptr = h->storageHWM;
  h->storageHWM += size;
  h->storageHWM += HEAP_GAP_MAX;  // Make a gap to help overrun detection.

  objHeader* header = h->headersHWM;
  h->headersHWM++;
  header->objType = type;
  header->size = size;
  header->ptr = ptr;
  
  return ptr;
}

void pool_free(pool* thisP){
  poolImpl* this = (poolImpl*)thisP;

  if(this->heaps){
    for(heap* h = this->heaps; h < this->heaps + this->numHeaps; h++){
      free(h->headers);
      free(h->storage);
    }
    free(this->heaps);
  }
  free(thisP);
}

pool* pool_new(){
  poolImpl* this = malloc(sizeof(poolImpl));

  this->public.alloc = &pool_alloc;
  this->public.free = &pool_free;

  this->numHeaps = 0;
  this->heaps = NULL;

  return (pool*)this;
}
