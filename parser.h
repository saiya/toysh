#pragma once

#include <unistd.h>

typedef struct commandLine{
  char* str;

  struct command* command;

  void (*free)(struct commandLine* this);
} commandLine;

typedef enum commandPipeType{
  NoPipe,
  NormalPipe,
} commandPipeType;
typedef enum commandType{
  EXEC,
  //  ASSIGN,
  //  EXPORT,
} commandType;
typedef struct command{
  commandType type;
  
  char* name;
  int argc;
  char** argv;

  commandPipeType pipeType;
  struct command* next;
} command;


/** Parse command line.
 * @param str  Command line.
 *             This function will copy given string.
 */
commandLine* commandLine_parse(const char* str);



typedef enum commandTokenType{
  T_String,
  T_Pipe,
  T_NULL,
} commandTokenType;
typedef struct commandToken{
  const char* str_start;
  /** strlen(token) == (str_end - str_start); str_end points next of last char. */
  const char* str_end;
  const char* str_next;
  
  commandTokenType type;
} commandToken;
int readToken(const char* str, commandToken* t);
command* command_parse(const char* str, commandToken* nextToken);
