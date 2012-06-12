#pragma once

#include "parser.h"


/*
typedef struct commandHandle{
  pid_t pid;
  
  int stdout_reader;
  int (*wait)(struct commandHandle* this);
  void (*freeSucc)(struct commandHandle* this);

  struct commandHandle* prev;
  struct commandHandle* next;
} commandHandle;
*/

typedef struct commandLineHandle {
  size_t procs;
  pid_t* pids;
  int* codes;

  void (*wait)(struct commandLineHandle* this);
} commandLineHandle;

commandLineHandle* toysh_start(const commandLine* cl, pool* p);
void toysh();
