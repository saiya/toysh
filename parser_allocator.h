#pragma once

typedef enum{
  O_Generic = 0,
  O_Command = 1,
  O_String = 2,
} poolObjType;

typedef struct pool{
  void* (*alloc)(struct pool* this, poolObjType type, size_t size);
  void (*free)(struct pool* this);
} pool;

pool* pool_new();
