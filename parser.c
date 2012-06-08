#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "parser.h"

void commandLine_free(commandLine* this){
  command* cmd = this->command;
  free(this->str);
  while(cmd){
    command* next = cmd->next;
    for(size_t i = 0; i < cmd->argc; i++){
      char* arg = *(cmd->argv + i);
      free(arg);
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

  commandToken* nextToken;
  command* last = command_parse(result->str, &nextToken);
  result->command = last;
  
  while(nextToken){
    switch(nextToken->type){
    case T_Pipe: break;
    default:
      puts("Invalid sequence...");
      result->free(result);
      return NULL;
    }
    
    last->next = command_parse(nextToken->str_next, &nextToken);
    last = last->next;
    last->pipeType = NormalPipe;
  }
  
  return result;
}


command* command_parse(const char* str, commandToken** nextToken){
  *nextToken = NULL;

  commandToken* t = readToken(str);
  if(! t){
    return NULL;
  }
  if(t->type != T_String){
    puts("Parse error.");
    return NULL;
  }

  struct command* command = (struct command*)malloc(sizeof(struct command));
  command->next = NULL;
  command->name = strndup(t->str_start, t->str_next - t->str_start);

  command->argc = 1;
  size_t argv_size = 1;
  command->argv = (char**)malloc((argv_size + 1) * sizeof(char*));
  command->argv[0] = strdup(command->name);
  
  while(1){
    t = readToken(t->str_next);
    if(! t) break;
    if(t->type != T_String) break;
 
    if(command->argc <= argv_size){
      argv_size *= 2;
      command->argv = realloc(command->argv, (argv_size + 1) * sizeof(char*));
    }

    command->argv[command->argc] = strndup(t->str_start, t->str_next - t->str_start);
    command->argc++;

    commandToken* oldToken = *nextToken;
    free(oldToken);
  }
  *nextToken = t;
  command->argv[command->argc] = NULL;

  return command;
}


commandToken* readToken(const char* str){
  while(isspace(*str)){ str++; }
  if(*str == '\0') return NULL;

  commandToken* result = (commandToken*)malloc(sizeof(commandToken));
  result->str_start = str;

  switch(*str){
  case '|':
    result->type = T_Pipe;
    result->str_next = str + 1;
    break;

  default:
    result->type = T_String;
    while(*str != '\0'){
      int eos = 0;
      switch(*result->str_start){
      case '\"':
	if(*str == '\"') eos = 1;
	break;
      case '\'':
	if(*str == '\'') eos = 1;
	break;
      default:
	if(isspace(*str)) eos = 1;
	break;
      }
      if(eos) break;

      str++;
    }
    result->str_next = str;
    break;
  }
  return result;
}
