#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <readline/history.h>
#include <readline/readline.h>
#include "toysh.h"
#include "parser.h"
#include "dictionary.h"


void commandHandle_freeSucc(commandHandle* this){
  commandHandle* current = this;
  while(current){
    commandHandle* next = current->next;
    free(current);
    current = next;
  }
}
int commandHandle_wait(commandHandle* this){
  int result;
  waitpid(this->pid, &result, 0);
  return result;
}

commandHandle* toysh_command_run(commandHandle* prev, const command* cmd){
  commandHandle* result = malloc(sizeof(commandHandle));
  result->stdout_reader = -1;
  result->wait = &commandHandle_wait;
  result->freeSucc = &commandHandle_freeSucc;
  result->prev = prev;
  if(prev) prev->next = result;
  result->next = NULL;

  int stdout_redirect_to = -1;
  if(cmd->next && cmd->next->pipeType == NormalPipe){
    int p[2];
    if(pipe(p) == -1){
      puts("pipe() failed.");
      return NULL;
    }
    
    result->stdout_reader = p[0];
    stdout_redirect_to = p[1];
  }

  result->pid = fork();
  switch(result->pid){
  case -1:  // fork failure!
    puts("fork() failed.");
    return NULL;

  case 0:   // Child
    if(result->stdout_reader != -1) close(result->stdout_reader);
    if(cmd->pipeType == NormalPipe) dup2(prev->stdout_reader, 0);
    if(stdout_redirect_to != -1) dup2(stdout_redirect_to, 1);

    execvp(cmd->name, cmd->argv);

    // Error
    printf("`%s` exec error (%d): %s\n", cmd->name, errno, strerror(errno));
    exit(EXIT_FAILURE);

  default:  // Parent
    if(stdout_redirect_to != -1) close(stdout_redirect_to);
    if(cmd->pipeType == NormalPipe) close(prev->stdout_reader);

    return result;
  }
}

void toysh_run(const commandLine* cline){
  if(! cline->command) return;
  
  command* cmd = cline->command;
  commandHandle* head = NULL;
  commandHandle* current = NULL;
  while(cmd != NULL){
    commandHandle* next = toysh_command_run(current, cmd);
    if(! next) break;  // Failed to run current command.
    if(! head) head = next;
    cmd = cmd->next;
    current = next;
  }

  for(current = head; current != NULL; current = current->next){
    current->wait(current);
  }
  
  head->freeSucc(head);
}





void toysh(){
  char *prompt = "toysh % "; // getenv("PS2");
  char *line = NULL;
  while((line = readline(prompt))){
    commandLine* cline = commandLine_parse(line);
    free(line);

    if(! cline){
      continue;
    }

    if(cline->command){
      // cline->str is clean string, remember it.
      add_history(cline->str);
    }
    
    toysh_run(cline);
    
    cline->free(cline);
  }
}
int main(){
  toysh();
}
