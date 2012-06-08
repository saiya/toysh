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


void commandLine_free(commandLine* this){
  command* cmd = this->command;
  free(this->str);
  while(cmd){
    command* next = cmd->next;
    for(size_t i = 0; i < cmd->argc; i++){
      free(cmd->argv + i);
    }
    free(cmd->argv);
    free(cmd);
    cmd = next;
  }
  free(this);
}
commandLine* commandLine_alloc(){
  commandLine* result = (commandLine*)malloc(sizeof(commandLine));
  result->free = &commandLine_free;
  return result;
}
commandLine* commandLine_parse(const char* str){
  commandLine* result = commandLine_alloc();
  
  const char* str_start = str;
  const char* str_end = str + strlen(str);
  while(str_start < str_end && isspace(*str_start)){ str_start++; }
  while(str_start < str_end && isspace(*str_end)){ str_end--; }
  if(str_start == str_end){
    // Given str is space only!
    result->str = (char*)malloc(1);
    result->str[0] = '\0';
    result->command = NULL;
    return result;
  }
  result->str = (char*)malloc(str_end - str_start + 1);
  strcpy(result->str, str_start);

  // FIXME: Implement (pipe, redirection...)
  result->command = (command*)malloc(sizeof(command));
  result->command->name = result->str;
  result->command->argc = 0;
  result->command->argv = (char**)malloc(2 * sizeof(char*));
  result->command->argv[0] = strdup(result->command->name);
  result->command->argv[1] = NULL;
  result->command->next = NULL;
  
  return result;
}



void toysh_run(const commandLine* cline){
  if(! cline->command) return;

  pid_t f = fork();
  int child_result;
  switch(f){
  case -1:  // fork failure!
    puts("fork failed.");
    break;
  case 0:   // Child
    execvp(cline->command->name, cline->command->argv);
    printf("`%s` exec error: %d, %s\n", cline->command->name, errno, strerror(errno));
    break;
  default:  // Parent
    waitpid(f, &child_result, 0);
    printf("%d\n", child_result);
    break;
  }
}



void toysh(){
  char *prompt = "toysh % "; // getenv("PS2");
  char *line = NULL;
  while(line = readline(prompt)){
    commandLine* cline = commandLine_parse(line);
    free(line);

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
