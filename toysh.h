#pragma once

#include "parser.h"


typedef struct commandHandle{
  pid_t pid;
  
  /** If has successor, stdout will be piped.
   * -1 is default.
   */
  int stdout_reader;

  /** Wait until process exit.
   * @return int  Process return code
   */
  int (*wait)(struct commandHandle* this);

  /** Free this and nexts (not prevs). */
  void (*freeSucc)(struct commandHandle* this);

  struct commandHandle* prev;
  struct commandHandle* next;
} commandHandle;

commandHandle* toysh_command_run(commandHandle* prev, const command* cmd);
void toysh_run(const commandLine* cline);
void toysh();
